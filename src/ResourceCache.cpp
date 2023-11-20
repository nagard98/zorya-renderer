#include "ResourceCache.h"
#include "RenderHardwareInterface.h"
#include "Shaders.h"
#include "Material.h"

#include <d3d11_1.h>
#include <wrl/client.h>
#include <cstdint>
#include <cassert>

namespace wrl = Microsoft::WRL;

ResourceCache resourceCache;

ResourceCache::ResourceCache()
{
}

ResourceCache::~ResourceCache()
{
}

MaterialCacheHandle_t ResourceCache::AllocMaterial(const MaterialDesc& matDesc, MaterialCacheHandle_t& matCacheHnd)
{
	Material* m;
	int matIndex = matCacheHnd.index;
	if ((matCacheHnd.isCached & IS_FIRST_MAT_ALLOC) == IS_FIRST_MAT_ALLOC) {
		materialCache.push_back(Material{});
		matIndex = materialCache.size() - 1;
	}

	m = &materialCache.at(matIndex);
	m->model.shader = shaders.pixelShaders.at((std::uint8_t)PShaderID::STANDARD);

	if ((matCacheHnd.isCached & UPDATE_MAT_MAPS)) {
		
		rhi.LoadTexture(matDesc.albedoPath, m->albedoMap);
		rhi.LoadTexture(matDesc.normalPath, m->normalMap, false);
		rhi.LoadTexture(matDesc.metalnessMask, m->metalnessMap);
	}

	if ((matCacheHnd.isCached & UPDATE_MAT_PRMS)) {
		m->matPrms.hasAlbedoMap = m->albedoMap.resourceView != nullptr;
		m->matPrms.hasNormalMap = m->normalMap.resourceView != nullptr;
		m->matPrms.hasMetalnessMap = m->metalnessMap.resourceView != nullptr;

		m->matPrms.baseColor = matDesc.baseColor;
		m->matPrms.metalness = matDesc.metalness;
		m->matPrms.smoothness = matDesc.smoothness;
	}

	return MaterialCacheHandle_t{ (std::uint16_t)matIndex, NO_UPDATE_MAT };
}

void ResourceCache::UpdateMaterialSmoothness(const MaterialCacheHandle_t matHnd, float smoothness)
{
	assert(matHnd.isCached);
	assert(smoothness >= 0.0f && smoothness <= 1.0f);
	Material& matCache = materialCache.at(matHnd.index);
	matCache.matPrms.smoothness = smoothness;
}

void ResourceCache::UpdateMaterialAlbedoMap(const MaterialCacheHandle_t matHnd, const wchar_t* albedoMapPath)
{
}
