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
}


RendererBackend::~RendererBackend()
{
    if (matPrmsCB) matPrmsCB->Release();
    if (lightsCB) lightsCB->Release();
    if (objectCB) objectCB->Release();
    if (viewCB) viewCB->Release();
    if (projCB) projCB->Release();
    if (shadowMap) shadowMap->Release();
}

HRESULT RendererBackend::Init()
{
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

    sm_viewport.TopLeftX = 0.0f;
    sm_viewport.TopLeftY = 0.0f;
    sm_viewport.Width = 1024.0f;
    sm_viewport.Height = 1024.f;
    sm_viewport.MinDepth = 0.0f;
    sm_viewport.MaxDepth = 1.0f;
    
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

    return S_OK;
}

void RendererBackend::RenderShadowMaps(const std::vector<SubmeshInfo>& submeshesInfo, DirShadowCB& dirShadowCB, OmniDirShadowCB& cbOmniDirShad ) {
    int numPointLights = ARRAYSIZE(pLights);
    int numSpotLights = ARRAYSIZE(spotLights);

    std::uint32_t strides[] = { sizeof(Vertex) };
    std::uint32_t offsets[] = { 0 };

    rhi.context->RSSetViewports(1, &sm_viewport);

    //Directional lights--------------------------------------------
    rhi.context->ClearDepthStencilView(shadowMapDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    rhi.context->OMSetRenderTargets(0, nullptr, shadowMapDSV);

    rhi.context->VSSetShader(shaders.vertexShaders.at((std::uint8_t)VShaderID::DEPTH), nullptr, 0);
    rhi.context->VSSetConstantBuffers(0, 1, &objectCB);
    rhi.context->VSSetConstantBuffers(1, 1, &viewCB);
    rhi.context->VSSetConstantBuffers(2, 1, &projCB);

    dx::XMVECTOR lightPos = dx::XMVectorMultiply(dx::XMVectorNegate(dLight.direction), dx::XMVectorSet(10.0f, 10.0f, 10.0f, 1.0f));
    dx::XMMATRIX dirLightVMat = dx::XMMatrixLookAtLH(lightPos, dx::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    dx::XMMATRIX dirLightPMat = dx::XMMatrixOrthographicLH(12.0f, 12.0f, 1.0f, 20.0f);

    dirShadowCB.dirPMat = dx::XMMatrixTranspose(dirLightPMat);
    dirShadowCB.dirVMat = dx::XMMatrixTranspose(dirLightVMat);

    ViewCB tmpVCB{ dx::XMMatrixTranspose(dirLightVMat) };
    ProjCB tmpPCB{ dx::XMMatrixTranspose(dirLightPMat) };

    rhi.context->UpdateSubresource(viewCB, 0, nullptr, &tmpVCB, 0, 0);
    rhi.context->UpdateSubresource(projCB, 0, nullptr, &tmpPCB, 0, 0);

    rhi.context->PSSetShader(nullptr, nullptr, 0);

    for (SubmeshInfo const& sbPair : submeshesInfo) {
        rhi.context->IASetVertexBuffers(0, 1, bufferCache.GetVertexBuffer(sbPair.bufferHnd).buffer.GetAddressOf(), strides, offsets);
        rhi.context->IASetIndexBuffer(bufferCache.GetIndexBuffer(sbPair.bufferHnd).buffer.Get(), DXGI_FORMAT_R16_UINT, 0);

        ObjCB tmpOCB{ dx::XMMatrixTranspose(sbPair.finalWorldTransf) };
        rhi.context->UpdateSubresource(objectCB, 0, nullptr, &tmpOCB, 0, 0);

        RHIState state = RHI_DEFAULT_STATE();
        RHI_RS_SET_CULL_BACK(state);
        RHI_OM_DS_SET_DEPTH_COMP_LESS(state);

        rhi.SetState(state);

        rhi.context->DrawIndexed(sbPair.submeshHnd.numIndexes, 0, 0);
    }
    //-------------------------------------------------------------------------------

    //Spot lights--------------------------------------------------------------------
    for (int i = 0; i < numSpotLights; i++) {
        cbOmniDirShad.spotLightProjMat[i] = dx::XMMatrixTranspose(dx::XMMatrixPerspectiveFovLH(std::acos(spotLights[i].cosCutoffAngle)*2.0f, 1.0f, 1.0f, 50.0f));  //multiply acos by 2, because cutoff angle is considered from center, not entire light angle
        cbOmniDirShad.spotLightViewMat[i] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(spotLights[i].posWorldSpace, spotLights[i].direction, dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));

        tmpVCB.viewMatrix = cbOmniDirShad.spotLightViewMat[i];
        tmpPCB.projMatrix = cbOmniDirShad.spotLightProjMat[i];
    }

    rhi.context->OMSetRenderTargets(0, nullptr, spotShadowMapDSV);
    rhi.context->ClearDepthStencilView(spotShadowMapDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    rhi.context->UpdateSubresource(viewCB, 0, nullptr, &tmpVCB, 0, 0);
    rhi.context->UpdateSubresource(projCB, 0, nullptr, &tmpPCB, 0, 0);

    for (SubmeshInfo const& sbPair : submeshesInfo) {
        rhi.context->IASetVertexBuffers(0, 1, bufferCache.GetVertexBuffer(sbPair.bufferHnd).buffer.GetAddressOf(), strides, offsets);
        rhi.context->IASetIndexBuffer(bufferCache.GetIndexBuffer(sbPair.bufferHnd).buffer.Get(), DXGI_FORMAT_R16_UINT, 0);

        ObjCB tmpOCB{ dx::XMMatrixTranspose(sbPair.finalWorldTransf) };
        rhi.context->UpdateSubresource(objectCB, 0, nullptr, &tmpOCB, 0, 0);

        RHIState state = RHI_DEFAULT_STATE();
        RHI_RS_SET_CULL_BACK(state);
        RHI_OM_DS_SET_DEPTH_COMP_LESS(state);

        rhi.SetState(state);

        rhi.context->DrawIndexed(sbPair.submeshHnd.numIndexes, 0, 0);
    }


    //Point lights-------------------------------------------------------------------
    cbOmniDirShad.dirPMat = dx::XMMatrixTranspose(dx::XMMatrixPerspectiveFovLH(dx::XM_PIDIV2, 1.0f, 0.5f, 20.0f));

    for (int i = 0; i < numPointLights; i++) {

        cbOmniDirShad.dirVMat[i * 6 + CUBEMAP::FACE_POSITIVE_X] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(pLights[i].posWorldSpace, dx::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
        cbOmniDirShad.dirVMat[i * 6 + CUBEMAP::FACE_NEGATIVE_X] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(pLights[i].posWorldSpace, dx::XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
        cbOmniDirShad.dirVMat[i * 6 + CUBEMAP::FACE_POSITIVE_Y] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(pLights[i].posWorldSpace, dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f)));
        cbOmniDirShad.dirVMat[i * 6 + CUBEMAP::FACE_NEGATIVE_Y] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(pLights[i].posWorldSpace, dx::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f)));
        cbOmniDirShad.dirVMat[i * 6 + CUBEMAP::FACE_POSITIVE_Z] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(pLights[i].posWorldSpace, dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
        cbOmniDirShad.dirVMat[i * 6 + CUBEMAP::FACE_NEGATIVE_Z] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(pLights[i].posWorldSpace, dx::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));

        for (int face = 0; face < 6; face++) {
            rhi.context->ClearDepthStencilView(shadowCubeMapDSV[i * 6 + face], D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        }
    }

    for (SubmeshInfo const& sbPair : submeshesInfo) {
        rhi.context->IASetVertexBuffers(0, 1, bufferCache.GetVertexBuffer(sbPair.bufferHnd).buffer.GetAddressOf(), strides, offsets);
        rhi.context->IASetIndexBuffer(bufferCache.GetIndexBuffer(sbPair.bufferHnd).buffer.Get(), DXGI_FORMAT_R16_UINT, 0);

        ObjCB tmpOCB{ dx::XMMatrixTranspose(sbPair.finalWorldTransf) };
        rhi.context->UpdateSubresource(objectCB, 0, nullptr, &tmpOCB, 0, 0);

        RHIState state = RHI_DEFAULT_STATE();
        RHI_RS_SET_CULL_BACK(state);
        RHI_OM_DS_SET_DEPTH_COMP_LESS(state);

        rhi.SetState(state);

        tmpPCB.projMatrix = cbOmniDirShad.dirPMat;
        rhi.context->UpdateSubresource(projCB, 0, nullptr, &tmpPCB, 0, 0);

        for (int i = 0; i < numPointLights; i++) {
            for (int face = 0; face < 6; face++) {
                tmpVCB.viewMatrix = cbOmniDirShad.dirVMat[i * 6 + face];
                rhi.context->UpdateSubresource(viewCB, 0, nullptr, &tmpVCB, 0, 0);

                rhi.context->OMSetRenderTargets(0, nullptr, shadowCubeMapDSV[i * 6 + face]);
                rhi.context->DrawIndexed(sbPair.submeshHnd.numIndexes, 0, 0);
            }
        }
    }
}


void RendererBackend::RenderView(const ViewDesc& viewDesc)
{
    //TODO: this is just temporary for development purposes
    int numPointLights = ARRAYSIZE(pLights);
    int numSpotLights = ARRAYSIZE(spotLights);

	std::uint32_t strides[] = { sizeof(Vertex) };
	std::uint32_t offsets[] = { 0 };
	
    //Rendering shadow maps-------------------------------------------------
    DirShadowCB dirShadowCB{};
    OmniDirShadowCB cbOmniDirShad{};

    RenderShadowMaps(viewDesc.submeshesInfo, dirShadowCB, cbOmniDirShad);
    //----------------------------------------------------------------------


    //Actual rendering-------------------------------------------------------
    rhi.context->RSSetViewports(1, &rhi.viewport);
    rhi.context->OMSetRenderTargets(1, rhi.renderTargetView.GetAddressOf(), rhi.depthStencilView.Get());

    rhi.context->VSSetShader(shaders.vertexShaders.at((std::uint8_t)VShaderID::STANDARD), nullptr, 0);
    rhi.context->VSSetConstantBuffers(0, 1, &objectCB);
    rhi.context->VSSetConstantBuffers(1, 1, &viewCB);
    rhi.context->VSSetConstantBuffers(2, 1, &projCB);
    rhi.context->VSSetConstantBuffers(3, 1, &lightsCB);
    rhi.context->VSSetConstantBuffers(4, 1, &dirShadCB);
    rhi.context->VSSetConstantBuffers(5, 1, &omniDirShadCB);

    ViewCB tmpVCB{ viewDesc.cam.getViewMatrix() };
    ProjCB tmpPCB{ viewDesc.cam.getProjMatrix() };

    LightCB tmpLCB;
    //TODO: fix here using member _viewMatrix because need matrix non transposed
    tmpLCB.dLight.direction = dx::XMVector4Transform(dLight.direction, viewDesc.cam._viewMatrix);
    for (int i = 0; i < numPointLights; i++) {
        tmpLCB.posViewSpace[i] = dx::XMVector4Transform(pLights[i].posWorldSpace, viewDesc.cam._viewMatrix);
        tmpLCB.pointLights[i].constant = pLights[i].constant;
        tmpLCB.pointLights[i].linear = pLights[i].linear;
        tmpLCB.pointLights[i].quadratic = pLights[i].quadratic;
    }
    tmpLCB.numPLights = numPointLights;

    for (int i = 0; i < numSpotLights; i++) {
        tmpLCB.spotLights[i].cosCutoffAngle = spotLights[i].cosCutoffAngle;
        tmpLCB.spotLights[i].direction = spotLights[i].direction;
        tmpLCB.spotLights[i].posWorldSpace = spotLights[i].posWorldSpace;

        tmpLCB.posSpotViewSpace[i] = dx::XMVector4Transform(spotLights[i].posWorldSpace, viewDesc.cam._viewMatrix);
        tmpLCB.dirSpotViewSpace[i] = dx::XMVector4Transform(spotLights[i].direction, viewDesc.cam._viewMatrix);
    }
    tmpLCB.numSpotLights = numSpotLights;

    rhi.context->UpdateSubresource(viewCB, 0, nullptr, &tmpVCB, 0, 0);
    rhi.context->UpdateSubresource(projCB, 0, nullptr, &tmpPCB, 0, 0);
    rhi.context->UpdateSubresource(lightsCB, 0, nullptr, &tmpLCB, 0, 0);
    rhi.context->UpdateSubresource(dirShadCB, 0, nullptr, &dirShadowCB, 0, 0);
    rhi.context->UpdateSubresource(omniDirShadCB, 0, nullptr, &cbOmniDirShad, 0, 0);

	for (SubmeshInfo const &sbPair : viewDesc.submeshesInfo) {
		rhi.context->IASetVertexBuffers(0, 1, bufferCache.GetVertexBuffer(sbPair.bufferHnd).buffer.GetAddressOf(), strides, offsets);
		rhi.context->IASetIndexBuffer(bufferCache.GetIndexBuffer(sbPair.bufferHnd).buffer.Get(), DXGI_FORMAT_R16_UINT, 0);

		Material &mat = resourceCache.materialCache.at(sbPair.matCacheHnd.index);
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

    ID3D11ShaderResourceView* nullSRV[] = { nullptr };
    rhi.context->PSSetShaderResources(4, 1, nullSRV);
    rhi.context->PSSetShaderResources(5, 1, nullSRV);
    rhi.context->PSSetShaderResources(6, 1, nullSRV);

}
