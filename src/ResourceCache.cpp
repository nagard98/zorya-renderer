#include "ResourceCache.h"
#include "RenderHardwareInterface.h"
#include "Shaders.h"

#include <d3d11_1.h>
#include <wrl/client.h>
#include <cstdint>

namespace wrl = Microsoft::WRL;

ResourceCache resourceCache;

ResourceCache::ResourceCache()
{
}

ResourceCache::~ResourceCache()
{
}

MaterialCacheHandle_t ResourceCache::AllocMaterial(const MaterialDesc& matDesc)
{
	Material m;
	rhi.LoadTexture(matDesc.albedoPath, m.albedoMap);
	rhi.LoadTexture(matDesc.normalPath, m.normalMap, false);
	rhi.LoadTexture(matDesc.metalnessMask, m.metalnessMap);

	m.matPrms.hasAlbedoMap = m.albedoMap.resourceView != nullptr;
	m.matPrms.hasNormalMap = m.normalMap.resourceView != nullptr;
	m.matPrms.hasMetalnessMap = m.metalnessMap.resourceView != nullptr;

	m.matPrms.baseColor = matDesc.baseColor;
	m.matPrms.metalness = matDesc.metalness;
	m.matPrms.smoothness = matDesc.smoothness;

	wrl::ComPtr<ID3DBlob> blob;
	//TODO:implement correctly instead of hard coding
	HRESULT hRes = LoadShader<ID3D11PixelShader>(L"./shaders/BasicPixelShader.hlsl", "ps", blob.GetAddressOf(), &m.model.shader, rhi.device.Get());

	//TODO::implement std::move
	materialCache.push_back(m);

	return MaterialCacheHandle_t{ (std::uint16_t)(materialCache.size() - 1), 1 };
}
