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
	matPrmsCB = nullptr;
    lightsCB = nullptr;
    objectCB = nullptr;
    viewCB = nullptr;
    projCB = nullptr;
}


RendererBackend::~RendererBackend()
{
    if(matPrmsCB) matPrmsCB->Release();
    if(lightsCB) lightsCB->Release();
    if(objectCB) objectCB->Release();
    if(viewCB) viewCB->Release();
    if(projCB) projCB->Release();
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

    rhi.device->CreateBuffer(&lightBuffDesc, nullptr, &lightsCB);
    rhi.context->PSSetConstantBuffers(0, 1, &lightsCB);
    //---------------------------------------------------------
	
    return S_OK;
}

void RendererBackend::RenderView(const ViewDesc& viewDesc)
{

	std::uint32_t strides[] = { sizeof(Vertex) };
	std::uint32_t offsets[] = { 0 };
	
    ViewCB tmpVCB{ viewDesc.cam.getViewMatrix() };
    rhi.context->UpdateSubresource(viewCB, 0, nullptr, &tmpVCB, 0, 0);

    ProjCB tmpPCB{ viewDesc.cam.getProjMatrix() };
    rhi.context->UpdateSubresource(projCB, 0, nullptr, &tmpPCB, 0, 0);

    LightCB tmpLCB;
    //TODO: fix here using member _viewMatrix because need matrix non transposed
    tmpLCB.dLight.direction = dx::XMVector4Transform(dLight.direction, viewDesc.cam._viewMatrix);
    tmpLCB.pointLights[0].pos = dx::XMVector4Transform(pLight1.pos, viewDesc.cam._viewMatrix);
    tmpLCB.pointLights[0].constant = pLight1.constant;
    tmpLCB.pointLights[0].linear = pLight1.linear;
    tmpLCB.pointLights[0].quadratic = pLight1.quadratic;
    tmpLCB.numPLights = 1;
    rhi.context->UpdateSubresource(lightsCB, 0, nullptr, &tmpLCB, 0, 0);

	for (SubmeshInfo const &sbPair : viewDesc.submeshesInfo) {
		rhi.context->IASetVertexBuffers(0, 1, bufferCache.GetVertexBuffer(sbPair.bufferHnd).buffer.GetAddressOf(), strides, offsets);
		rhi.context->IASetIndexBuffer(bufferCache.GetIndexBuffer(sbPair.bufferHnd).buffer.Get(), DXGI_FORMAT_R16_UINT, 0);

		Material &mat = resourceCache.materialCache.at(sbPair.matCacheHnd.index);
		rhi.context->PSSetShaderResources(0, 1, &mat.albedoMap.resourceView);
		rhi.context->PSSetShaderResources(1, 1, &mat.normalMap.resourceView);
        rhi.context->PSSetShaderResources(2, 1, &mat.metalnessMap.resourceView);
        rhi.context->PSSetShaderResources(3, 1, &mat.smoothnessMap.resourceView);
		rhi.context->PSSetShader(mat.model.shader, 0, 0);

		rhi.context->UpdateSubresource(matPrmsCB, 0, nullptr, &mat.matPrms, 0, 0);
		rhi.context->PSSetConstantBuffers(1, 1, &matPrmsCB);

        ObjCB tmpOCB{ dx::XMMatrixTranspose(sbPair.finalWorldTransf) };
        rhi.context->UpdateSubresource(objectCB, 0, nullptr, &tmpOCB, 0, 0);

		RHIState state = RHI_DEFAULT_STATE();
		RHI_RS_SET_CULL_BACK(state);
		RHI_OM_DS_SET_DEPTH_COMP_LESS(state);

		rhi.SetState(state);

		rhi.context->DrawIndexed(sbPair.submeshHnd.numIndexes, 0, 0);
	}

}
