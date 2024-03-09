#include "ResourceCache.h"
#include "RenderHardwareInterface.h"
#include "Shaders.h"
#include "Material.h"

#include <d3d11_1.h>
#include <wrl/client.h>
#include <cstdint>
#include <cassert>
#include <variant>
#include <cstdlib>
#include <iostream>
#include <random>

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
	m->model.shader = shaders.pixelShaders.at((std::uint8_t)matDesc.shaderType);

	if ((matCacheHnd.isCached & UPDATE_MAT_MAPS)) {
		rhi.LoadTexture(matDesc.albedoPath, m->albedoMap, false);
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
		m->matPrms.subsurfaceAlbedo = matDesc.subsurfaceAlbedo;
		m->matPrms.meanFreePathColor = matDesc.meanFreePathColor; 
		m->matPrms.meanFreePathDist = matDesc.meanFreePathDistance; //from cm to m
		m->matPrms.scale = fmax(0.0f, matDesc.scale);

		srand((int)matDesc.scale);

		std::random_device rd;
		std::mt19937 gen(6.0f);
		std::uniform_real_distribution<> dis(0, 1.0);//uniform distribution between 0 and 1
		for (int i = 0; i < 64; i++) {
			m->matPrms.samples[i] = dis(gen);
			OutputDebugString((std::to_string(m->matPrms.samples[i]).append(", ")).c_str());
		}

		std::sort(m->matPrms.samples, m->matPrms.samples + (int)(m->matPrms.scale * 4.0f));
		std::sort(m->matPrms.samples + (int)(m->matPrms.scale * 4.0f), m->matPrms.samples + (int)(m->matPrms.scale * 4.0f) + (int)(std::trunc(matDesc.subsurfaceAlbedo.y * 255.0f)*4.0f));

		//for (int i = 0; i < 64; i++) {
		//	m->matPrms.samples[i] = (float)(rand() % 10000) / 10000.0f;
		//}

		if ((matDesc.unionTags & METALNESS_IS_MAP) == 0) {
			m->matPrms.metalness = matDesc.metalnessValue;
			m->matPrms.hasMetalnessMap = false;
		}

		if ((matDesc.unionTags & SMOOTHNESS_IS_MAP) == 0) {
			m->matPrms.roughness = 1.0f - matDesc.smoothnessValue;
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
	matCache.matPrms.roughness = smoothness;
}

void ResourceCache::UpdateMaterialAlbedoMap(const MaterialCacheHandle_t matHnd, const wchar_t* albedoMapPath)
{
}
