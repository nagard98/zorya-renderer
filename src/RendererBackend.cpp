#include "RendererBackend.h"
#include "RendererFrontend.h"
#include "RenderHardwareInterface.h"
#include "RHIState.h"
#include "BufferCache.h"
#include "ResourceCache.h"
#include "Material.h"
#include "Lights.h"
#include "Camera.h"

#include <d3d11_1.h>
#include <cassert>

RendererBackend rb;

RendererBackend::RendererBackend()
{
    sm_viewport = D3D11_VIEWPORT{};

	matPrmsCB = nullptr;
    lightsCB = nullptr;
    objectCB = nullptr;
    viewCB = nullptr;
    projCB = nullptr;
    dirShadCB = nullptr;
    omniDirShadCB = nullptr;

    shadowMap = nullptr;
    shadowMapDSV = nullptr;
    shadowMapSRV = nullptr;
    
    shadowCubeMap = nullptr;
    shadowCubeMapSRV = nullptr;
    for (int i = 0; i < 6; i++) {
        shadowCubeMapDSV[i] = nullptr;
    }

    skinRT[0] = nullptr;
    skinRT[1] = nullptr;
    skinMaps[0] = nullptr;
    skinMaps[1] = nullptr;
    skinSRV[0] = nullptr;
    skinSRV[1] = nullptr;

    for (int i = 0; i < 3; i++) {
        skinRT[i] = nullptr;
        skinMaps[i] = nullptr;
        skinSRV[i] = nullptr;
    }

    for (int i = 0; i < GBuffer::SIZE; i++) {
        GBufferSRV[i] = nullptr;
        GBufferRTV[i] = nullptr;
        GBuffer[i] = nullptr;
    }

    invMatCB = nullptr;

    annot = nullptr;
}


RendererBackend::~RendererBackend()
{
    if (matPrmsCB) matPrmsCB->Release();
    if (lightsCB) lightsCB->Release();
    if (objectCB) objectCB->Release();
    if (viewCB) viewCB->Release();
    if (projCB) projCB->Release();
    if (shadowMap) shadowMap->Release();

    for (int i = 0; i < 3; i++) {
        if (skinSRV[i]) skinSRV[i]->Release();
        if (skinRT[i]) skinRT[i]->Release();
        if (skinMaps[i]) skinMaps[i]->Release();
    }

    for (int i = 0; i < GBuffer::SIZE; i++) {
        if (GBufferSRV[i]) GBufferSRV[i]->Release();
        if (GBufferRTV[i]) GBufferRTV[i]->Release();
        if (GBuffer[i]) GBuffer[i]->Release();
    }

    if (invMatCB) invMatCB->Release();
    if (annot) annot->Release();
}

HRESULT RendererBackend::Init()
{
    rhi.context->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)&annot);

    sm_viewport.TopLeftX = 0.0f;
    sm_viewport.TopLeftY = 0.0f;
    sm_viewport.Width = 2048.0f;
    sm_viewport.Height = 2048.f;
    sm_viewport.MinDepth = 0.0f;
    sm_viewport.MaxDepth = 1.0f;

	D3D11_BUFFER_DESC matCbDesc;
	ZeroMemory(&matCbDesc, sizeof(matCbDesc));
	matCbDesc.ByteWidth = sizeof(MaterialParams);
	matCbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matCbDesc.Usage = D3D11_USAGE_DEFAULT;

	HRESULT hr = rhi.device->CreateBuffer(&matCbDesc, nullptr, &matPrmsCB);
    
    RETURN_IF_FAILED(hr);

    //World transform constant buffer setup---------------------------------------------------
    D3D11_BUFFER_DESC cbObjDesc;
    cbObjDesc.Usage = D3D11_USAGE_DEFAULT;
    cbObjDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbObjDesc.CPUAccessFlags = 0;
    cbObjDesc.MiscFlags = 0;
    cbObjDesc.ByteWidth = sizeof(ObjCB);

    hr = rhi.device->CreateBuffer(&cbObjDesc, nullptr, &objectCB);
    RETURN_IF_FAILED(hr);

    rhi.context->VSSetConstantBuffers(0, 1, &objectCB);
    //---------------------------------------------------


    //View matrix constant buffer setup-------------------------------------------------------------
    D3D11_BUFFER_DESC cbCamDesc;
    cbCamDesc.Usage = D3D11_USAGE_DEFAULT;
    cbCamDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbCamDesc.CPUAccessFlags = 0;
    cbCamDesc.MiscFlags = 0;
    cbCamDesc.ByteWidth = sizeof(ViewCB);

    hr = rhi.device->CreateBuffer(&cbCamDesc, nullptr, &viewCB);
    RETURN_IF_FAILED(hr);

    rhi.context->VSSetConstantBuffers(1, 1, &viewCB);
    //----------------------------------------------------------------


    //Projection matrix constant buffer setup--------------------------------------
    D3D11_BUFFER_DESC cbProjDesc;
    cbProjDesc.Usage = D3D11_USAGE_DEFAULT;
    cbProjDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbProjDesc.CPUAccessFlags = 0;
    cbProjDesc.MiscFlags = 0;
    cbProjDesc.ByteWidth = sizeof(ProjCB);

    hr = rhi.device->CreateBuffer(&cbProjDesc, nullptr, &projCB);
    RETURN_IF_FAILED(hr);

    rhi.context->VSSetConstantBuffers(2, 1, &projCB);
    //------------------------------------------------------------------

    //Light constant buffer setup---------------------------------------
    D3D11_BUFFER_DESC lightBuffDesc;
    ZeroMemory(&lightBuffDesc, sizeof(lightBuffDesc));
    lightBuffDesc.Usage = D3D11_USAGE_DEFAULT;
    lightBuffDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    lightBuffDesc.ByteWidth = sizeof(LightCB);

    hr = rhi.device->CreateBuffer(&lightBuffDesc, nullptr, &lightsCB);
    RETURN_IF_FAILED(hr);

    rhi.context->PSSetConstantBuffers(0, 1, &lightsCB);
    rhi.context->VSSetConstantBuffers(3, 1, &lightsCB);
    //---------------------------------------------------------
	
    //GBuffer setup--------------------------------------------
    
    D3D11_TEXTURE2D_DESC gbufferDesc;
    gbufferDesc.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
    gbufferDesc.ArraySize = 1;
    gbufferDesc.MipLevels = 1;
    gbufferDesc.Usage = D3D11_USAGE_DEFAULT;
    gbufferDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    gbufferDesc.CPUAccessFlags = 0;
    gbufferDesc.MiscFlags = 0;
    gbufferDesc.SampleDesc.Count = 1;
    gbufferDesc.SampleDesc.Quality = 0;
    gbufferDesc.Width = 1280.0f;
    gbufferDesc.Height = 720.0f;

    for (int i = 0; i < GBuffer::SIZE; i++) {
        hr = rhi.device->CreateTexture2D(&gbufferDesc, nullptr, &GBuffer[i]);
        RETURN_IF_FAILED(hr);
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC gBufferSRVDesc;
    gBufferSRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    gBufferSRVDesc.Texture2D.MipLevels = 1;
    gBufferSRVDesc.Texture2D.MostDetailedMip = 0;
    gBufferSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

    for (int i = 0; i < GBuffer::SIZE; i++) {
        hr = rhi.device->CreateShaderResourceView(GBuffer[i], &gBufferSRVDesc, &GBufferSRV[i]);
        RETURN_IF_FAILED(hr);
    }

    D3D11_RENDER_TARGET_VIEW_DESC gBufferRTVDesc;
    gBufferRTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    gBufferRTVDesc.Texture2D.MipSlice = 0;
    gBufferRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

    for (int i = 0; i < GBuffer::SIZE; i++) {
        hr = rhi.device->CreateRenderTargetView(GBuffer[i], &gBufferRTVDesc, &GBufferRTV[i]);
        RETURN_IF_FAILED(hr);
    }


    D3D11_BUFFER_DESC invMatDesc;
    ZeroMemory(&invMatDesc, sizeof(invMatDesc));
    invMatDesc.ByteWidth = sizeof(dx::XMMatrixIdentity()) * 2;
    invMatDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    invMatDesc.Usage = D3D11_USAGE_DEFAULT;

    hr = rhi.device->CreateBuffer(&invMatDesc, nullptr, &invMatCB);

    //---------------------------------------------------------

    //Shadow map setup-------------------------------------------------
    D3D11_BUFFER_DESC dirShadowMatBuffDesc;
    ZeroMemory(&dirShadowMatBuffDesc, sizeof(D3D11_BUFFER_DESC));
    dirShadowMatBuffDesc.Usage = D3D11_USAGE_DEFAULT;
    dirShadowMatBuffDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    dirShadowMatBuffDesc.ByteWidth = sizeof(DirShadowCB);

    hr = rhi.device->CreateBuffer(&dirShadowMatBuffDesc, nullptr, &dirShadCB);
    RETURN_IF_FAILED(hr);

    D3D11_BUFFER_DESC omniDirShadowMatBuffDesc;
    ZeroMemory(&omniDirShadowMatBuffDesc, sizeof(D3D11_BUFFER_DESC));
    omniDirShadowMatBuffDesc.Usage = D3D11_USAGE_DEFAULT;
    omniDirShadowMatBuffDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    omniDirShadowMatBuffDesc.ByteWidth = sizeof(OmniDirShadowCB);

    hr = rhi.device->CreateBuffer(&omniDirShadowMatBuffDesc, nullptr, &omniDirShadCB);
    RETURN_IF_FAILED(hr);
    
    D3D11_TEXTURE2D_DESC sm_texDesc;
    ZeroMemory(&sm_texDesc, sizeof(sm_texDesc));
    sm_texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    sm_texDesc.ArraySize = 1;
    sm_texDesc.MipLevels = 1;
    sm_texDesc.Usage = D3D11_USAGE_DEFAULT;
    sm_texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
    sm_texDesc.Height = sm_viewport.Height;
    sm_texDesc.Width = sm_viewport.Width;
    sm_texDesc.SampleDesc.Count = 1;
    sm_texDesc.SampleDesc.Quality = 0;

    D3D11_DEPTH_STENCIL_VIEW_DESC sm_dsvDesc;
    sm_dsvDesc.Texture2D.MipSlice = 0;
    sm_dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    sm_dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    sm_dsvDesc.Flags = 0;

    D3D11_SHADER_RESOURCE_VIEW_DESC sm_srvDesc;
    sm_srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    sm_srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    sm_srvDesc.Texture2D.MostDetailedMip = 0;
    sm_srvDesc.Texture2D.MipLevels = -1;

    hr = rhi.device->CreateTexture2D(&sm_texDesc, nullptr, &shadowMap);
    RETURN_IF_FAILED(hr);
    hr = rhi.device->CreateTexture2D(&sm_texDesc, nullptr, &spotShadowMap);
    RETURN_IF_FAILED(hr);

    hr = rhi.device->CreateShaderResourceView(shadowMap, &sm_srvDesc, &shadowMapSRV);
    RETURN_IF_FAILED(hr);
    hr = rhi.device->CreateShaderResourceView(spotShadowMap, &sm_srvDesc, &spotShadowMapSRV);
    RETURN_IF_FAILED(hr);

    hr = rhi.device->CreateDepthStencilView(shadowMap, &sm_dsvDesc, &shadowMapDSV);
    RETURN_IF_FAILED(hr);
    hr = rhi.device->CreateDepthStencilView(spotShadowMap, &sm_dsvDesc, &spotShadowMapDSV);
    RETURN_IF_FAILED(hr);

    D3D11_TEXTURE2D_DESC sm_cubeTexDesc;
    ZeroMemory(&sm_cubeTexDesc, sizeof(sm_cubeTexDesc));
    sm_cubeTexDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    sm_cubeTexDesc.ArraySize = 12;
    sm_cubeTexDesc.MipLevels = 1;
    sm_cubeTexDesc.Usage = D3D11_USAGE_DEFAULT;
    sm_cubeTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
    sm_cubeTexDesc.Height = sm_viewport.Height;
    sm_cubeTexDesc.Width = sm_viewport.Width;
    sm_cubeTexDesc.SampleDesc.Count = 1;
    sm_cubeTexDesc.SampleDesc.Quality = 0;
    sm_cubeTexDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;


    D3D11_SHADER_RESOURCE_VIEW_DESC sm_cubeSrvDesc;
    sm_cubeSrvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    sm_cubeSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    sm_cubeSrvDesc.Texture2DArray.ArraySize = 12;
    sm_cubeSrvDesc.Texture2DArray.FirstArraySlice = 0;
    sm_cubeSrvDesc.Texture2DArray.MipLevels = 1;
    sm_cubeSrvDesc.Texture2DArray.MostDetailedMip = 0;


    hr = rhi.device->CreateTexture2D(&sm_cubeTexDesc, nullptr, &shadowCubeMap);
    RETURN_IF_FAILED(hr);
    hr = rhi.device->CreateShaderResourceView(shadowCubeMap, &sm_cubeSrvDesc, &shadowCubeMapSRV);
    RETURN_IF_FAILED(hr);

    D3D11_DEPTH_STENCIL_VIEW_DESC sm_cubeDsvDesc;
    int numScmDSV = ARRAYSIZE(shadowCubeMapDSV);
    for (int i = 0; i < numScmDSV; i++)
    {
        sm_cubeDsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        sm_cubeDsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        sm_cubeDsvDesc.Texture2DArray.MipSlice = 0;
        sm_cubeDsvDesc.Texture2DArray.ArraySize = 1;
        sm_cubeDsvDesc.Texture2DArray.FirstArraySlice = i;
        sm_cubeDsvDesc.Flags = 0;

        hr = rhi.device->CreateDepthStencilView(shadowCubeMap, &sm_cubeDsvDesc, &shadowCubeMapDSV[i]);
        RETURN_IF_FAILED(hr);
    }
    
    //-----------------------------------------------------------------

    //Irradiance map setup---------------------------------------------
    
    D3D11_TEXTURE2D_DESC irradTexDesc;
    irradTexDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    irradTexDesc.MipLevels = 0;
    irradTexDesc.ArraySize = 1;
    irradTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    irradTexDesc.CPUAccessFlags = 0;
    irradTexDesc.MiscFlags = 0;
    irradTexDesc.Height = 720;
    irradTexDesc.Width = 1280;
    irradTexDesc.Usage = D3D11_USAGE_DEFAULT;
    irradTexDesc.SampleDesc.Count = 1;
    irradTexDesc.SampleDesc.Quality = 0;
    
    for (int i = 0; i < 3; i++) {
        hr = rhi.device->CreateTexture2D(&irradTexDesc, nullptr, &skinMaps[i]);
        RETURN_IF_FAILED(hr);

        hr = rhi.device->CreateRenderTargetView(skinMaps[i], NULL, &skinRT[i]);
        RETURN_IF_FAILED(hr);

        hr = rhi.device->CreateShaderResourceView(skinMaps[i], NULL, &skinSRV[i]);
        RETURN_IF_FAILED(hr);
    }
    //hr = rhi.device->CreateTexture2D(&irradTexDesc, nullptr, &skinMaps[0]);
    //RETURN_IF_FAILED(hr);
    //hr = rhi.device->CreateTexture2D(&irradTexDesc, nullptr, &skinMaps[1]);
    //RETURN_IF_FAILED(hr);

    //hr = rhi.device->CreateRenderTargetView(skinMaps[0], NULL, &skinRT[0]);
    //RETURN_IF_FAILED(hr);
    //hr = rhi.device->CreateRenderTargetView(skinMaps[1], NULL, &skinRT[1]);
    //RETURN_IF_FAILED(hr);

    //hr = rhi.device->CreateShaderResourceView(skinMaps[0], NULL, &skinSRV[0]);
    //RETURN_IF_FAILED(hr);
    //hr = rhi.device->CreateShaderResourceView(skinMaps[1], NULL, &skinSRV[1]);
    //RETURN_IF_FAILED(hr);    

    //-----------------------------------------------------------------

    return S_OK;
}

void RendererBackend::RenderShadowMaps(const ViewDesc& viewDesc, DirShadowCB& dirShadowCB, OmniDirShadowCB& cbOmniDirShad ) {
    annot->BeginEvent(L"ShadowMap Pre-Pass");
    {
        std::vector<LightInfo> dirLights;
        std::vector<LightInfo> spotLights;
        std::vector<LightInfo> pointLights;

        ViewCB tmpVCB;
        ProjCB tmpPCB;
        ObjCB tmpOCB;

        std::uint32_t strides[] = { sizeof(Vertex) };
        std::uint32_t offsets[] = { 0 };

        rhi.context->RSSetViewports(1, &sm_viewport);

        for (const LightInfo& lightInfo : viewDesc.lightsInfo) {
            switch (lightInfo.tag)
            {
                case LightType::DIRECTIONAL:
                {
                    dirLights.push_back(lightInfo);
                    break;
                }

                case LightType::SPOT:
                {
                    spotLights.push_back(lightInfo);
                    break;
                }

                case LightType::POINT:
                {
                    pointLights.push_back(lightInfo);
                    break;
                }
            }
        }

        int numSpotLights = spotLights.size();
        assert(numSpotLights == viewDesc.numSpotLights);

        int numPointLights = pointLights.size();
        assert(numPointLights == viewDesc.numPointLights);

        int numDirLigths = dirLights.size();
        assert(numDirLigths == viewDesc.numDirLights);

        rhi.context->PSSetShader(nullptr, nullptr, 0);
        rhi.context->VSSetShader(shaders.vertexShaders.at((std::uint8_t)VShaderID::DEPTH), nullptr, 0);

        //Directional lights--------------------------------------------
        annot->BeginEvent(L"Directional Lights");

        rhi.context->ClearDepthStencilView(shadowMapDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        rhi.context->OMSetRenderTargets(0, nullptr, shadowMapDSV);

        rhi.context->VSSetConstantBuffers(0, 1, &objectCB);
        rhi.context->VSSetConstantBuffers(1, 1, &viewCB);
        rhi.context->VSSetConstantBuffers(2, 1, &projCB);

        for (int i = 0; i < numDirLigths; i++)
        {
            DirectionalLight& dirLight = dirLights.at(i).dirLight;
            dx::XMVECTOR transfDirLight = dx::XMVector4Transform(dirLight.direction, dirLights.at(i).finalWorldTransf);
            dx::XMVECTOR lightPos = dx::XMVectorMultiply(dx::XMVectorNegate(transfDirLight), dx::XMVectorSet(10.0f, 10.0f, 10.0f, 1.0f));
            //TODO: rename these matrices; it isnt clear that they are used for shadow mapping
            dx::XMMATRIX dirLightVMat = dx::XMMatrixLookAtLH(lightPos, dx::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
            dx::XMMATRIX dirLightPMat = dx::XMMatrixOrthographicLH(12.0f, 12.0f, dirLight.shadowMapNearPlane, dirLight.shadowMapFarPlane);

            dirShadowCB.dirPMat = dx::XMMatrixTranspose(dirLightPMat);
            dirShadowCB.dirVMat = dx::XMMatrixTranspose(dirLightVMat);
            cbOmniDirShad.dirLightProjMat = dirShadowCB.dirPMat;
            cbOmniDirShad.dirLightViewMat = dirShadowCB.dirVMat;

            tmpVCB.viewMatrix = dx::XMMatrixTranspose(dirLightVMat);
            tmpPCB.projMatrix = dx::XMMatrixTranspose(dirLightPMat);

            rhi.context->UpdateSubresource(viewCB, 0, nullptr, &tmpVCB, 0, 0);
            rhi.context->UpdateSubresource(projCB, 0, nullptr, &tmpPCB, 0, 0);

            for (SubmeshInfo const& sbPair : viewDesc.submeshesInfo) {
                rhi.context->IASetVertexBuffers(0, 1, bufferCache.GetVertexBuffer(sbPair.bufferHnd).buffer.GetAddressOf(), strides, offsets);
                rhi.context->IASetIndexBuffer(bufferCache.GetIndexBuffer(sbPair.bufferHnd).buffer.Get(), DXGI_FORMAT_R16_UINT, 0);

                tmpOCB.worldMatrix = dx::XMMatrixTranspose(sbPair.finalWorldTransf);
                rhi.context->UpdateSubresource(objectCB, 0, nullptr, &tmpOCB, 0, 0);

                RHIState state = RHI_DEFAULT_STATE();
                RHI_RS_SET_CULL_BACK(state);
                RHI_OM_DS_SET_DEPTH_COMP_LESS(state);

                rhi.SetState(state);

                rhi.context->DrawIndexed(sbPair.submeshHnd.numIndexes, 0, 0);
            }
        }
        annot->EndEvent();

        //-------------------------------------------------------------------------------

        //Spot lights--------------------------------------------------------------------
        annot->BeginEvent(L"Spot Lights");

        rhi.context->OMSetRenderTargets(0, nullptr, spotShadowMapDSV);
        rhi.context->ClearDepthStencilView(spotShadowMapDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        for (int i = 0; i < numSpotLights; i++) {
            dx::XMVECTOR finalPos = dx::XMVector4Transform(spotLights[i].spotLight.posWorldSpace, spotLights[i].finalWorldTransf);
            dx::XMVECTOR finalDir = dx::XMVector4Transform(spotLights[i].spotLight.direction, spotLights[i].finalWorldTransf);
            cbOmniDirShad.spotLightProjMat[i] = dx::XMMatrixTranspose(dx::XMMatrixPerspectiveFovLH(std::acos(spotLights[i].spotLight.cosCutoffAngle) * 2.0f, 1.0f, spotLights[i].spotLight.shadowMapNearPlane, spotLights[i].spotLight.shadowMapFarPlane));  //multiply acos by 2, because cutoff angle is considered from center, not entire light angle
            cbOmniDirShad.spotLightViewMat[i] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(finalPos, finalDir, dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));

            tmpVCB.viewMatrix = cbOmniDirShad.spotLightViewMat[i];
            tmpPCB.projMatrix = cbOmniDirShad.spotLightProjMat[i];

            rhi.context->UpdateSubresource(viewCB, 0, nullptr, &tmpVCB, 0, 0);
            rhi.context->UpdateSubresource(projCB, 0, nullptr, &tmpPCB, 0, 0);

            for (SubmeshInfo const& sbPair : viewDesc.submeshesInfo) {
                rhi.context->IASetVertexBuffers(0, 1, bufferCache.GetVertexBuffer(sbPair.bufferHnd).buffer.GetAddressOf(), strides, offsets);
                rhi.context->IASetIndexBuffer(bufferCache.GetIndexBuffer(sbPair.bufferHnd).buffer.Get(), DXGI_FORMAT_R16_UINT, 0);

                tmpOCB.worldMatrix = dx::XMMatrixTranspose(sbPair.finalWorldTransf);
                rhi.context->UpdateSubresource(objectCB, 0, nullptr, &tmpOCB, 0, 0);

                RHIState state = RHI_DEFAULT_STATE();
                RHI_RS_SET_CULL_BACK(state);
                RHI_OM_DS_SET_DEPTH_COMP_LESS(state);

                rhi.SetState(state);

                rhi.context->DrawIndexed(sbPair.submeshHnd.numIndexes, 0, 0);
            }
        }
        annot->EndEvent();

        //Point lights-------------------------------------------------------------------
        //TODO: do something about this; shouldnt hard code index in pointLights
        annot->BeginEvent(L"Point Lights");

        if (numPointLights > 0) cbOmniDirShad.pointLightProjMat = dx::XMMatrixTranspose(dx::XMMatrixPerspectiveFovLH(dx::XM_PIDIV2, 1.0f, pointLights[0].pointLight.shadowMapNearPlane, pointLights[0].pointLight.shadowMapFarPlane));

        for (int i = 0; i < numPointLights; i++) {
            dx::XMVECTOR finalPos = dx::XMVector4Transform(pointLights[i].pointLight.posWorldSpace, pointLights[i].finalWorldTransf);
            cbOmniDirShad.pointLightViewMat[i * 6 + CUBEMAP::FACE_POSITIVE_X] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(finalPos, dx::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
            cbOmniDirShad.pointLightViewMat[i * 6 + CUBEMAP::FACE_NEGATIVE_X] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(finalPos, dx::XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
            cbOmniDirShad.pointLightViewMat[i * 6 + CUBEMAP::FACE_POSITIVE_Y] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(finalPos, dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f)));
            cbOmniDirShad.pointLightViewMat[i * 6 + CUBEMAP::FACE_NEGATIVE_Y] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(finalPos, dx::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f)));
            cbOmniDirShad.pointLightViewMat[i * 6 + CUBEMAP::FACE_POSITIVE_Z] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(finalPos, dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
            cbOmniDirShad.pointLightViewMat[i * 6 + CUBEMAP::FACE_NEGATIVE_Z] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(finalPos, dx::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));

            for (int face = 0; face < 6; face++) {
                rhi.context->ClearDepthStencilView(shadowCubeMapDSV[i * 6 + face], D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
            }

            for (SubmeshInfo const& sbPair : viewDesc.submeshesInfo) {
                rhi.context->IASetVertexBuffers(0, 1, bufferCache.GetVertexBuffer(sbPair.bufferHnd).buffer.GetAddressOf(), strides, offsets);
                rhi.context->IASetIndexBuffer(bufferCache.GetIndexBuffer(sbPair.bufferHnd).buffer.Get(), DXGI_FORMAT_R16_UINT, 0);

                ObjCB tmpOCB{ dx::XMMatrixTranspose(sbPair.finalWorldTransf) };
                rhi.context->UpdateSubresource(objectCB, 0, nullptr, &tmpOCB, 0, 0);

                RHIState state = RHI_DEFAULT_STATE();
                RHI_RS_SET_CULL_BACK(state);
                RHI_OM_DS_SET_DEPTH_COMP_LESS(state);

                rhi.SetState(state);

                tmpPCB.projMatrix = cbOmniDirShad.pointLightProjMat;
                rhi.context->UpdateSubresource(projCB, 0, nullptr, &tmpPCB, 0, 0);

                for (int face = 0; face < 6; face++) {
                    tmpVCB.viewMatrix = cbOmniDirShad.pointLightViewMat[i * 6 + face];
                    rhi.context->UpdateSubresource(viewCB, 0, nullptr, &tmpVCB, 0, 0);

                    rhi.context->OMSetRenderTargets(0, nullptr, shadowCubeMapDSV[i * 6 + face]);
                    rhi.context->DrawIndexed(sbPair.submeshHnd.numIndexes, 0, 0);
                }

            }
        }
        annot->EndEvent();

    }
    annot->EndEvent();
}


void RendererBackend::RenderView(const ViewDesc& viewDesc)
{
	std::uint32_t strides[] = { sizeof(Vertex) };
	std::uint32_t offsets[] = { 0 };
	
    std::vector<LightInfo> dirLights;
    std::vector<LightInfo> spotLights;
    std::vector<LightInfo> pointLights;
    //TODO: remove this code here and in RenderShadowMaps; instead order the array of lights in buckets of same type 
    for (const LightInfo& lightInfo : viewDesc.lightsInfo) {
        switch (lightInfo.tag)
        {
            case LightType::DIRECTIONAL:
            {
                dirLights.push_back(lightInfo);
                break;
            }

            case LightType::SPOT:
            {
                spotLights.push_back(lightInfo);
                break;
            }

            case LightType::POINT:
            {
                pointLights.push_back(lightInfo);
                break;
            }
        }
    }

    int numSpotLights = spotLights.size();
    assert(numSpotLights == viewDesc.numSpotLights);

    int numPointLights = pointLights.size();
    assert(numPointLights == viewDesc.numPointLights);

    int numDirLigths = dirLights.size();
    assert(numDirLigths == viewDesc.numDirLights);

    rhi.context->VSSetConstantBuffers(0, 1, &objectCB);
    rhi.context->VSSetConstantBuffers(1, 1, &viewCB);
    rhi.context->VSSetConstantBuffers(2, 1, &projCB);

    rhi.context->IASetInputLayout(shaders.vertexLayout);

    //Rendering shadow maps-------------------------------------------------
    DirShadowCB dirShadowCB{};
    OmniDirShadowCB cbOmniDirShad{};

    RenderShadowMaps(viewDesc, dirShadowCB, cbOmniDirShad);
    //----------------------------------------------------------------------


    //Actual rendering-------------------------------------------------------
    rhi.context->RSSetViewports(1, &rhi.viewport);

    ViewCB tmpVCB{ viewDesc.cam.getViewMatrixTransposed() };
    ProjCB tmpPCB{ viewDesc.cam.getProjMatrixTransposed() };
    LightCB tmpLCB;

    for (int i = 0; i < numDirLigths; i++)
    {
        DirectionalLight& dirLight = dirLights.at(i).dirLight;
        dx::XMVECTOR transfDirLight = dx::XMVector4Transform(dirLight.direction, dirLights.at(i).finalWorldTransf);
        tmpLCB.dLight.direction = dx::XMVector4Transform(transfDirLight, viewDesc.cam.getViewMatrix());
        tmpLCB.dLight.shadowMapNearPlane = dirLight.shadowMapNearPlane;
        tmpLCB.dLight.shadowMapFarPlane = dirLight.shadowMapFarPlane;
    }

    for (int i = 0; i < numPointLights; i++) {
        dx::XMVECTOR finalPos = dx::XMVector4Transform(pointLights[i].pointLight.posWorldSpace, pointLights[i].finalWorldTransf);
        tmpLCB.posViewSpace[i] = dx::XMVector4Transform(finalPos, viewDesc.cam.getViewMatrix());
        tmpLCB.pointLights[i].constant = pointLights[i].pointLight.constant;
        tmpLCB.pointLights[i].linear = pointLights[i].pointLight.linear;
        tmpLCB.pointLights[i].quadratic = pointLights[i].pointLight.quadratic;
        tmpLCB.pointLights[i].shadowMapNearPlane = pointLights[i].pointLight.shadowMapNearPlane;
        tmpLCB.pointLights[i].shadowMapFarPlane = pointLights[i].pointLight.shadowMapFarPlane;
    }
    tmpLCB.numPLights = numPointLights;

    for (int i = 0; i < viewDesc.numSpotLights; i++) {
        dx::XMVECTOR finalPos = dx::XMVector4Transform(spotLights[i].spotLight.posWorldSpace, spotLights[i].finalWorldTransf);
        dx::XMVECTOR finalDir = dx::XMVector4Transform(spotLights[i].spotLight.direction, spotLights[i].finalWorldTransf);

        tmpLCB.spotLights[i].cosCutoffAngle = spotLights[i].spotLight.cosCutoffAngle;
        tmpLCB.spotLights[i].posWorldSpace = finalPos;
        tmpLCB.spotLights[i].direction = finalDir;
        tmpLCB.spotLights[i].shadowMapNearPlane = spotLights[i].spotLight.shadowMapNearPlane;
        tmpLCB.spotLights[i].shadowMapFarPlane = spotLights[i].spotLight.shadowMapFarPlane;

        tmpLCB.posSpotViewSpace[i] = dx::XMVector4Transform(finalPos, viewDesc.cam.getViewMatrix());
        tmpLCB.dirSpotViewSpace[i] = dx::XMVector4Transform(finalDir, viewDesc.cam.getViewMatrix());
    }
    tmpLCB.numSpotLights = numSpotLights;

    rhi.context->UpdateSubresource(viewCB, 0, nullptr, &tmpVCB, 0, 0);
    rhi.context->UpdateSubresource(projCB, 0, nullptr, &tmpPCB, 0, 0);
    rhi.context->UpdateSubresource(lightsCB, 0, nullptr, &tmpLCB, 0, 0);
    rhi.context->UpdateSubresource(dirShadCB, 0, nullptr, &dirShadowCB, 0, 0);
    rhi.context->UpdateSubresource(omniDirShadCB, 0, nullptr, &cbOmniDirShad, 0, 0);

    annot->BeginEvent(L"G-Buffer Pass");
    {
        rhi.context->VSSetShader(shaders.vertexShaders.at((std::uint8_t)VShaderID::STANDARD), nullptr, 0);
        rhi.context->VSSetConstantBuffers(0, 1, &objectCB);
        rhi.context->VSSetConstantBuffers(1, 1, &viewCB);
        rhi.context->VSSetConstantBuffers(2, 1, &projCB);
        rhi.context->VSSetConstantBuffers(3, 1, &lightsCB);
        rhi.context->VSSetConstantBuffers(4, 1, &dirShadCB);
        rhi.context->VSSetConstantBuffers(5, 1, &omniDirShadCB);

        FLOAT clearCol[4] = { 0.0f,0.0f,0.0f,1.0f };
        for (int i = 0; i < GBuffer::SIZE; i++) {
            rhi.context->ClearRenderTargetView(GBufferRTV[i], clearCol);
        }

        rhi.context->OMSetRenderTargets(3, &GBufferRTV[0], rhi.depthStencilView.Get());

        for (SubmeshInfo const& sbPair : viewDesc.submeshesInfo) {
            rhi.context->IASetVertexBuffers(0, 1, bufferCache.GetVertexBuffer(sbPair.bufferHnd).buffer.GetAddressOf(), strides, offsets);
            rhi.context->IASetIndexBuffer(bufferCache.GetIndexBuffer(sbPair.bufferHnd).buffer.Get(), DXGI_FORMAT_R16_UINT, 0);

            Material& mat = resourceCache.materialCache.at(sbPair.matCacheHnd.index);
            rhi.context->PSSetShader(mat.model.shader, 0, 0);
            rhi.context->PSSetShaderResources(0, 1, &mat.albedoMap.resourceView);
            rhi.context->PSSetShaderResources(1, 1, &mat.normalMap.resourceView);
            rhi.context->PSSetShaderResources(2, 1, &mat.metalnessMap.resourceView);
            rhi.context->PSSetShaderResources(3, 1, &mat.smoothnessMap.resourceView);
            rhi.context->PSSetShaderResources(4, 1, &shadowMapSRV);
            rhi.context->PSSetShaderResources(5, 1, &shadowCubeMapSRV);
            rhi.context->PSSetShaderResources(6, 1, &spotShadowMapSRV);

            rhi.context->UpdateSubresource(matPrmsCB, 0, nullptr, &mat.matPrms, 0, 0);
            rhi.context->PSSetConstantBuffers(0, 1, &lightsCB);
            rhi.context->PSSetConstantBuffers(1, 1, &matPrmsCB);
            rhi.context->PSSetConstantBuffers(2, 1, &omniDirShadCB);

            ObjCB tmpOCB{ dx::XMMatrixTranspose(sbPair.finalWorldTransf) };
            rhi.context->UpdateSubresource(objectCB, 0, nullptr, &tmpOCB, 0, 0);

            RHIState state = RHI_DEFAULT_STATE();
            RHI_RS_SET_CULL_BACK(state);
            RHI_OM_DS_SET_DEPTH_COMP_LESS(state);

            rhi.SetState(state);

            rhi.context->DrawIndexed(sbPair.submeshHnd.numIndexes, 0, 0);
        }
    }
    annot->EndEvent();


    annot->BeginEvent(L"Lighting Pass");
    {
        rhi.context->OMSetRenderTargets(3, &skinRT[0], nullptr);

        rhi.context->VSSetShader(shaders.vertexShaders.at((std::uint8_t)VShaderID::FULL_QUAD), nullptr, 0);
        rhi.context->IASetInputLayout(nullptr);
        rhi.context->IASetVertexBuffers(0, 0, NULL, strides, offsets);
        rhi.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        rhi.context->PSSetShader(shaders.pixelShaders.at((std::uint8_t)PShaderID::LIGHTING), nullptr, 0);
        struct Im { dx::XMMATRIX invProj; dx::XMMATRIX invView; };
        Im im;
        im.invProj = dx::XMMatrixTranspose(dx::XMMatrixInverse(nullptr, viewDesc.cam.getProjMatrix()));
        im.invView = dx::XMMatrixTranspose(dx::XMMatrixInverse(nullptr, viewDesc.cam.getViewMatrix()));
        rhi.context->UpdateSubresource(invMatCB, 0, nullptr, &im, 0, 0);
        rhi.context->PSSetConstantBuffers(0, 1, &lightsCB);
        rhi.context->PSSetConstantBuffers(5, 1, &invMatCB);
        rhi.context->PSSetShaderResources(0, 3, &GBufferSRV[0]);
        rhi.context->PSSetShaderResources(3, 1, rhi.depthStencilShaderResourceView.GetAddressOf());
        rhi.context->PSSetShaderResources(4, 1, &shadowMapSRV);
        rhi.context->PSSetShaderResources(5, 1, &shadowCubeMapSRV);
        rhi.context->PSSetShaderResources(6, 1, &spotShadowMapSRV);
        rhi.context->Draw(4, 0);
    }
    annot->EndEvent();

    ID3D11ShaderResourceView* nullSRV[7] = { nullptr };

    annot->BeginEvent(L"ShadowMap Pass");
    {
        rhi.context->PSSetShaderResources(GBuffer::ALBEDO, 1, &nullSRV[0]);
        rhi.context->PSSetShaderResources(GBuffer::ROUGH_MET, 1, &nullSRV[0]);
        ID3D11RenderTargetView* tmpRT[2] = { GBufferRTV[GBuffer::ALBEDO], GBufferRTV[GBuffer::ROUGH_MET] };
        rhi.context->OMSetRenderTargets(2, &tmpRT[0], nullptr);
        rhi.context->PSSetShaderResources(0, 1, &skinSRV[0]);
        rhi.context->PSSetShaderResources(2, 1, &skinSRV[1]);
        rhi.context->PSSetShader(shaders.pixelShaders.at((std::uint8_t)PShaderID::SHADOW_MAP), nullptr, 0);
        rhi.context->Draw(4, 0);
    }
    annot->EndEvent();

    annot->BeginEvent(L"SSSSS Pass");
    {
        rhi.context->OMSetRenderTargets(1, rhi.renderTargetView.GetAddressOf(), nullptr);
        rhi.context->PSSetShader(shaders.pixelShaders.at((std::uint8_t)PShaderID::SSSSS), nullptr, 0);
        //NOTA: Gbuffer riutilizzato, non con valori originali
        rhi.context->PSSetShaderResources(0, 1, &GBufferSRV[GBuffer::ALBEDO]);
        rhi.context->PSSetShaderResources(1, 1, &GBufferSRV[GBuffer::ROUGH_MET]);
        rhi.context->PSSetShaderResources(2, 1, &skinSRV[2]);
        rhi.context->PSSetShaderResources(3, 1, rhi.depthStencilShaderResourceView.GetAddressOf());
        rhi.context->Draw(4, 0);
    }
    annot->EndEvent();

    //rhi.context->OMSetRenderTargets(1, rhi.renderTargetView.GetAddressOf(), nullptr);
    //rhi.context->PSSetShader(shaders.pixelShaders.at((std::uint8_t)PShaderID::PRESENT), nullptr, 0);
    //rhi.context->PSSetShaderResources(0, 1, &GBufferSRV[GBuffer::ALBEDO]);
    //rhi.context->Draw(4, 0);


    rhi.context->PSSetShaderResources(0, 7, &nullSRV[0]);

    rhi.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //rhi.context->ClearDepthStencilView(rhi.depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    rhi.context->OMSetRenderTargets(1, rhi.renderTargetView.GetAddressOf(), rhi.depthStencilView.Get());
}
