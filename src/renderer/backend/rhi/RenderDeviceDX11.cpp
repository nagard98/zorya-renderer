#include "RenderDevice.h"

#include <wrl/client.h>
#include <cstdint>
#include <d3d11_1.h>
#include <vector>
#include <cassert>

namespace wrl = Microsoft::WRL;


#define SOFT_LIMIT_RESOURCE_TYPE 256

DX11RenderDevice::DX11RenderDevice() :
	tex2DResources(SOFT_LIMIT_RESOURCE_TYPE),
	rtvResources(SOFT_LIMIT_RESOURCE_TYPE),
	srvResources(SOFT_LIMIT_RESOURCE_TYPE),
	dsvResources(SOFT_LIMIT_RESOURCE_TYPE),
	tex2dCount(0),
	rtvCount(0),
	srvCount(0),
	dsvCount(0) {}

DX11RenderDevice::~DX11RenderDevice()
{

}

ZRYResult DX11RenderDevice::createTex2D(RenderTextureHandle* texHandle, ZRYBindFlags bindFlags, ZRYFormat format, float width, float height, int numTex, RenderSRVHandle* srvHandle, RenderRTVHandle* rtvHandle, bool generateMips, int mipLevels, int arraySize, int sampleCount, int sampleQuality) {
	assert(texHandle != nullptr);

	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	texDesc.Format = format.value;
	texDesc.MipLevels = mipLevels;
	texDesc.ArraySize = arraySize;
	texDesc.BindFlags = bindFlags.value; //(srvHandle != nullptr ? D3D11_BIND_SHADER_RESOURCE : 0) | (rtvHandle != nullptr ? D3D11_BIND_RENDER_TARGET : 0);

	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.SampleDesc.Count = sampleCount;
	texDesc.SampleDesc.Quality = sampleQuality;

	//TODO:usage/access_flags/misc_flags
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = generateMips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

	ZRYResult zr{ S_OK };
	for (int i = 0; i < numTex; i++) {
		zr.value = device->CreateTexture2D(&texDesc, nullptr, &tex2DResources.at(tex2dCount));
		RETURN_IF_FAILED_ZRY(zr);
		//TODO: better handle creation for resources

		if (srvHandle != nullptr) {
			zr.value = device->CreateShaderResourceView(tex2DResources.at(tex2dCount), nullptr, &srvResources.at(srvCount));
			RETURN_IF_FAILED_ZRY(zr);
			//TODO: better handle creation for resources
			(srvHandle + i)->index = srvCount;
			srvCount += 1;
		}
		if (rtvHandle != nullptr) {
			zr.value = device->CreateRenderTargetView(tex2DResources.at(tex2dCount), nullptr, &rtvResources.at(rtvCount));
			RETURN_IF_FAILED_ZRY(zr);
			//TODO: better handle creation for resources
			(rtvHandle + i)->index = rtvCount;
			rtvCount += 1;
		}

		(texHandle + i)->index = tex2dCount;
		tex2dCount += 1;
	}

	return zr;
}

ZRYResult DX11RenderDevice::createSRVTex2D(RenderSRVHandle* srvHandle, const RenderTextureHandle* texHandle, ZRYFormat format, int numSRVs, int mipLevels, int mostDetailedMip)
{
	assert(srvHandle != nullptr);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));

	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = format.value;
	srvDesc.Texture2D.MipLevels = mipLevels;
	srvDesc.Texture2D.MostDetailedMip = mostDetailedMip;

	ZRYResult zr{ S_OK };
	for (int i = 0; i < numSRVs; i++) {
		zr.value = device->CreateShaderResourceView(getTex2DPointer(*(texHandle + i)), &srvDesc, &srvResources.at(srvCount));
		RETURN_IF_FAILED_ZRY(zr);
		(srvHandle + i)->index = srvCount;
		srvCount += 1;
	}

	return zr;
}

ZRYResult DX11RenderDevice::createRTVTex2D(RenderRTVHandle* rtvHandle, const RenderTextureHandle* texHandle, ZRYFormat format, int numRTVs, int mipSlice)
{
	assert(rtvHandle != nullptr);

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	ZeroMemory(&rtvDesc, sizeof(rtvDesc));

	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = format.value;
	rtvDesc.Texture2D.MipSlice = mipSlice;

	ZRYResult zr{ S_OK };
	for (int i = 0; i < numRTVs; i++) {
		zr.value = device->CreateRenderTargetView(getTex2DPointer(*(texHandle + i)), &rtvDesc, &rtvResources.at(rtvCount));
		RETURN_IF_FAILED_ZRY(zr);
		(rtvHandle + i)->index = rtvCount;
		rtvCount += 1;
	}

	return zr;
}

ZRYResult DX11RenderDevice::createDSVTex2D(RenderDSVHandle* dsvHandle, const RenderTextureHandle* texHandle, ZRYFormat format, int numDSVs, int mipSlice)
{
	assert(dsvHandle != nullptr);

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	ZeroMemory(&dsvDesc, sizeof(dsvDesc));

	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = format.value;
	dsvDesc.Texture2D.MipSlice = mipSlice;

	ZRYResult zr{ S_OK };
	for (int i = 0; i < numDSVs; i++) {
		zr.value = device->CreateDepthStencilView(getTex2DPointer(*(texHandle + i)), &dsvDesc, &dsvResources.at(dsvCount));
		RETURN_IF_FAILED_ZRY(zr);
		(dsvHandle + i)->index = dsvCount;
		dsvCount += 1;
	}

	return zr;
}

ZRYResult DX11RenderDevice::createTexCubemap(RenderTextureHandle* texHandle, ZRYBindFlags bindFlags, ZRYFormat format, float width, float height, int numTex, RenderSRVHandle* srvHandle, RenderRTVHandle* rtvHandle, bool generateMips, int mipLevels, int arraySize, int sampleCount, int sampleQuality)
{
	assert(texHandle != nullptr);

	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	texDesc.Format = format.value;
	texDesc.MipLevels = mipLevels;
	texDesc.ArraySize = arraySize;
	texDesc.BindFlags = bindFlags.value; //(srvHandle != nullptr ? D3D11_BIND_SHADER_RESOURCE : 0) | (rtvHandle != nullptr ? D3D11_BIND_RENDER_TARGET : 0);

	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.SampleDesc.Count = sampleCount;
	texDesc.SampleDesc.Quality = sampleQuality;

	//TODO:usage/access_flags/misc_flags
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | (generateMips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0);

	ZRYResult zr{ S_OK };
	for (int i = 0; i < numTex; i++) {
		zr.value = device->CreateTexture2D(&texDesc, nullptr, &tex2DResources.at(tex2dCount));
		RETURN_IF_FAILED_ZRY(zr);
		//TODO: better handle creation for resources

		if (srvHandle != nullptr) {
			zr.value = device->CreateShaderResourceView(tex2DResources.at(tex2dCount), nullptr, &srvResources.at(srvCount));
			RETURN_IF_FAILED_ZRY(zr);
			//TODO: better handle creation for resources
			(srvHandle + i)->index = srvCount;
			srvCount += 1;
		}
		if (rtvHandle != nullptr) {
			zr.value = device->CreateRenderTargetView(tex2DResources.at(tex2dCount), nullptr, &rtvResources.at(rtvCount));
			RETURN_IF_FAILED_ZRY(zr);
			//TODO: better handle creation for resources
			(rtvHandle + i)->index = rtvCount;
			rtvCount += 1;
		}

		(texHandle + i)->index = tex2dCount;
		tex2dCount += 1;
	}

	return zr;
}

ZRYResult DX11RenderDevice::createSRVTex2DArray(RenderSRVHandle* srvHandle, const RenderTextureHandle* texHandle, ZRYFormat format, int numSRVs, int arraySize, int firstArraySlice, int mipLevels, int mostDetailedMip)
{
	assert(srvHandle);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));

	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Format = format.value;
	srvDesc.Texture2DArray.MipLevels = mipLevels;
	srvDesc.Texture2DArray.MostDetailedMip = mostDetailedMip;
	srvDesc.Texture2DArray.ArraySize = arraySize;
	srvDesc.Texture2DArray.FirstArraySlice = firstArraySlice;

	ZRYResult zr{ S_OK };
	for (int i = 0; i < numSRVs; i++) {
		zr.value = device->CreateShaderResourceView(getTex2DPointer(*(texHandle + i)), &srvDesc, &srvResources.at(srvCount));
		RETURN_IF_FAILED_ZRY(zr);
		(srvHandle + i)->index = srvCount;
		srvCount += 1;
	}

	return zr;
}

ZRYResult DX11RenderDevice::createDSVTex2DArray(RenderDSVHandle* dsvHandle, const RenderTextureHandle* texHandle, ZRYFormat format, int arraySize, int numDSVs, int mipSlice, int firstArraySlice)
{
	assert(dsvHandle != nullptr);

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	ZeroMemory(&dsvDesc, sizeof(dsvDesc));

	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
	dsvDesc.Format = format.value;
	dsvDesc.Texture2DArray.ArraySize = arraySize;
	dsvDesc.Texture2DArray.FirstArraySlice = firstArraySlice;
	dsvDesc.Texture2DArray.MipSlice = mipSlice;

	ZRYResult zr{S_OK};
	for (int i = 0; i < numDSVs; i++) {
		zr.value = device->CreateDepthStencilView(getTex2DPointer(*(texHandle + i)), &dsvDesc, &dsvResources.at(dsvCount));
		RETURN_IF_FAILED_ZRY(zr);
		(dsvHandle + i)->index = dsvCount;
		dsvCount += 1;
	}

	return zr;
}

ID3D11Texture2D* DX11RenderDevice::getTex2DPointer(const RenderTextureHandle rtHnd) const
{
	return tex2DResources.at(rtHnd.index);
}

ID3D11RenderTargetView* DX11RenderDevice::getRTVPointer(const RenderRTVHandle rtvHnd) const
{
	return rtvResources.at(rtvHnd.index);
}

ID3D11ShaderResourceView* DX11RenderDevice::getSRVPointer(const RenderSRVHandle srvHnd) const
{
	return srvResources.at(srvHnd.index);
}

ID3D11DepthStencilView* DX11RenderDevice::getDSVPointer(const RenderDSVHandle dsvHnd) const
{
	return dsvResources.at(dsvHnd.index);;
}

void DX11RenderDevice::releaseAllResources()
{
	for (size_t i = 0; i < srvCount; i++)
	{
		ID3D11ShaderResourceView* resource = srvResources.at(i);
		if (resource)resource->Release();
	}
	srvCount = 0;

	for (size_t i = 0; i < rtvCount; i++)
	{
		ID3D11RenderTargetView* resource = rtvResources.at(i);
		if (resource)resource->Release();
	}
	rtvCount = 0;

	for (size_t i = 0; i < dsvCount; i++)
	{
		ID3D11DepthStencilView* resource = dsvResources.at(i);
		if (resource)resource->Release();
	}
	dsvCount = 0;

	for (size_t i = 0; i < tex2dCount; i++)
	{
		ID3D11Texture2D* resource = tex2DResources.at(i);
		if (resource)resource->Release();
	}

	tex2dCount = 0;

	for (size_t i = 0; i < constBuffCount; i++)
	{
		ConstantBuffer& resource = constantBuffers.at(i);
		if (resource.buffer) resource.buffer->Release();
	}

	constBuffCount = 0;
}
