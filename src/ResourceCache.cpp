#include "ResourceCache.h"
#include "RenderHardwareInterface.h"
#include "Shaders.h"
#include "Material.h"

#include <d3d11_1.h>
#include <wrl/client.h>
#include <cstdint>
#include <cassert>
#include <variant>

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
		if ((matDesc.unionTags & METALNESS_IS_MAP) == METALNESS_IS_MAP) {
			rhi.LoadTexture(matDesc.metalnessMap, m->metalnessMap, false);
			m->matPrms.hasMetalnessMap = true;
		}
		if ((matDesc.unionTags & SMOOTHNESS_IS_MAP) == SMOOTHNESS_IS_MAP) {
			rhi.LoadTexture(matDesc.smoothnessMap, m->smoothnessMap, false);
			m->matPrms.hasSmoothnessMap = true;
		}
	}

	if ((matCacheHnd.isCached & UPDATE_MAT_PRMS)) {
		m->matPrms.hasAlbedoMap = m->albedoMap.resourceView != nullptr;
		m->matPrms.hasNormalMap = m->normalMap.resourceView != nullptr;
		m->matPrms.hasMetalnessMap = m->metalnessMap.resourceView != nullptr;

		m->matPrms.baseColor = matDesc.baseColor;

		if ((matDesc.unionTags & METALNESS_IS_MAP) == 0) {
			m->matPrms.metalness = matDesc.metalnessValue;
			m->matPrms.hasMetalnessMap = false;
		}
		//TODO: change matPrms name smoothness to roughness
		if ((matDesc.unionTags & SMOOTHNESS_IS_MAP) == 0) {
			m->matPrms.smoothness = 1.0f - matDesc.smoothnessValue;
			m->matPrms.hasSmoothnessMap = false;
		}

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
