#include "RendererBackend.h"
#include "RendererFrontend.h"
#include "RenderHardwareInterface.h"
#include "RHIState.h"
#include "BufferCache.h"
#include "ResourceCache.h"
#include "Material.h"
#include <d3d11_1.h>

RendererBackend::RendererBackend()
{
	matPrmsCB = nullptr;
}


RendererBackend::~RendererBackend()
{
}

void RendererBackend::Init()
{

	D3D11_BUFFER_DESC matCbDesc;
	ZeroMemory(&matCbDesc, sizeof(matCbDesc));
	matCbDesc.ByteWidth = sizeof(MaterialParams);
	matCbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matCbDesc.Usage = D3D11_USAGE_DEFAULT;


	HRESULT res = rhi.device->CreateBuffer(&matCbDesc, nullptr, &matPrmsCB);
	
}

void RendererBackend::RenderView(const ViewDesc& viewDesc, float smoothness)
{

	std::uint32_t strides[] = { sizeof(Vertex) };
	std::uint32_t offsets[] = { 0 };
	
	for (SubmeshInfo const &sbPair : viewDesc.submeshBufferPairs) {
		rhi.context->IASetVertexBuffers(0, 1, bufferCache.GetVertexBuffer(sbPair.bufferHnd).buffer.GetAddressOf(), strides, offsets);
		rhi.context->IASetIndexBuffer(bufferCache.GetIndexBuffer(sbPair.bufferHnd).buffer.Get(), DXGI_FORMAT_R16_UINT, 0);

		Material &mat = resourceCache.materialCache.at(sbPair.matHnd.index);
		rhi.context->PSSetShaderResources(0, 1, &mat.albedoMap.resourceView);
		rhi.context->PSSetShaderResources(1, 1, &mat.normalMap.resourceView);
		rhi.context->PSSetShader(mat.model.shader, 0, 0);

		mat.matPrms.smoothness = smoothness;
		rhi.context->UpdateSubresource(matPrmsCB, 0, nullptr, &mat.matPrms, 0, 0);
		rhi.context->PSSetConstantBuffers(1, 1, &matPrmsCB);

		RHIState state = RHI_DEFAULT_STATE();
		RHI_RS_SET_CULL_BACK(state);
		RHI_OM_DS_SET_DEPTH_COMP_LESS_EQ(state);

		rhi.SetState(state);

		rhi.context->DrawIndexed(sbPair.submeshHnd.numIndexes, 0, 0);
	}

}
