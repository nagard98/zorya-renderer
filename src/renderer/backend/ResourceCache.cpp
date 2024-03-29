#include "renderer/backend/ResourceCache.h"
#include "renderer/backend/rhi/RenderHardwareInterface.h"

#include "renderer/frontend/Shader.h"
#include "renderer/frontend/Material.h"

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

void ResourceCache::ReleaseAllResources()
{
	for (Material& mat : materialCache) {
		if (mat.albedoMap.resourceView) mat.albedoMap.resourceView->Release();
		if (mat.smoothnessMap.resourceView) mat.metalnessMap.resourceView->Release();
		if (mat.smoothnessMap.resourceView) mat.smoothnessMap.resourceView->Release();
		mat.model.freeShader();
	}
}

MaterialCacheHandle_t ResourceCache::AllocMaterial(const ReflectionBase* matDesc, MaterialCacheHandle_t& matCacheHnd)
{
	Material* m;
	auto& standardMaterialDesc = static_cast<ReflectionContainer<StandardMaterialDesc>*>(const_cast<ReflectionBase*>(matDesc))->reflectedStruct;
	int matIndex = matCacheHnd.index;

	if ((matCacheHnd.isCached & IS_FIRST_MAT_ALLOC) == IS_FIRST_MAT_ALLOC) {
		if (freeMaterialCacheIndices.size() > 0) {
			matIndex = freeMaterialCacheIndices.back();
			freeMaterialCacheIndices.pop_back();
		}
		else {
			materialCache.push_back(Material{});
			matIndex = materialCache.size() - 1;
		}

		m = &materialCache.at(matIndex);
		m->model = PixelShader::create(standardMaterialDesc.shaderType);
	}
	else {
		m = &materialCache.at(matIndex);
	}

	//m->model.shader = shaders.pixelShaders.at((std::uint8_t)standardMaterialDesc.shaderType);
	//ShaderBytecode shaderBytecode = shaders.pixelShaderBytecodeBuffers[(std::uint8_t)standardMaterialDesc.shaderType];

	//m->matPrms.hasAlbedoMap = false;
	//m->matPrms.hasMetalnessMap = false;
	//m->matPrms.hasNormalMap = false;
	//m->matPrms.hasSmoothnessMap = false;

	if ((matCacheHnd.isCached & UPDATE_MAT_MAPS)) {
		rhi.LoadTexture(standardMaterialDesc.albedoPath, m->albedoMap, false);
		rhi.LoadTexture(standardMaterialDesc.normalPath, m->normalMap, false);
		if ((standardMaterialDesc.unionTags & METALNESS_IS_MAP) == METALNESS_IS_MAP) {
			rhi.LoadTexture(standardMaterialDesc.metalnessMap, m->metalnessMap, false);
			m->matPrms.hasMetalnessMap = true;
		}
		if ((standardMaterialDesc.unionTags & SMOOTHNESS_IS_MAP) == SMOOTHNESS_IS_MAP) {
			rhi.LoadTexture(standardMaterialDesc.smoothnessMap, m->smoothnessMap, false);
			m->matPrms.hasSmoothnessMap = true;
		}
	}

	if ((matCacheHnd.isCached & UPDATE_MAT_PRMS)) {
		m->matPrms.hasAlbedoMap = m->albedoMap.resourceView != nullptr;
		m->matPrms.hasNormalMap = m->normalMap.resourceView != nullptr;
		m->matPrms.hasMetalnessMap = m->metalnessMap.resourceView != nullptr;

		m->matPrms.baseColor = standardMaterialDesc.baseColor;
		m->matPrms.sssModelId = (int)standardMaterialDesc.selectedSSSModel;

		switch (standardMaterialDesc.selectedSSSModel) {
		case SSS_MODEL::GOLUBEV: 
		{
			m->sssPrms.subsurfaceAlbedo = standardMaterialDesc.sssModel.subsurfaceAlbedo;
			m->sssPrms.meanFreePathColor = standardMaterialDesc.sssModel.meanFreePathColor;
			m->sssPrms.meanFreePathDist = standardMaterialDesc.sssModel.meanFreePathDistance; //from cm to m
			m->sssPrms.scale = fmax(0.0f, standardMaterialDesc.sssModel.scale);
			m->sssPrms.numSamples = max(1, standardMaterialDesc.sssModel.numSamples);

			srand(42);

			std::random_device rd;
			std::mt19937 gen(6.0f);
			std::uniform_real_distribution<> dis(0, 1.0);//uniform distribution between 0 and 1
			for (int i = 0; i < m->sssPrms.numSamples; i++) {
				m->sssPrms.samples[i] = dis(gen);
				OutputDebugString((std::to_string(m->sssPrms.samples[i]).append(", ")).c_str());
			}

			std::sort(m->sssPrms.samples, m->sssPrms.samples + (m->sssPrms.numSamples - 1)); //(int)(max(0.0f, m->sssPrms.scale * 4.0f)));
			//std::sort(m->sssPrms.samples + (int)(max(0.0f, m->sssPrms.scale * 4.0f)), m->sssPrms.samples + (int)(max(0.0f, m->sssPrms.scale * 4.0f)) + (int)(std::trunc(max(0.0f, standardMaterialDesc.sssModel.subsurfaceAlbedo.y) * 255.0f) * 4.0f));

			//for (int i = 0; i < 64; i++) {
			//	m->matPrms.samples[i] = (float)(rand() % 10000) / 10000.0f;
			//}

			break;
		}
		default:
		{
			m->sssPrms.scale = fmax(0.0f, standardMaterialDesc.sssModel.scale);
			m->sssPrms.meanFreePathDist = standardMaterialDesc.sssModel.meanFreePathDistance; //from cm to m
			break;
		}
			
		}

		if ((standardMaterialDesc.unionTags & METALNESS_IS_MAP) == 0) {
			m->matPrms.metalness = standardMaterialDesc.metalnessValue;
			m->matPrms.hasMetalnessMap = false;
		}

		if ((standardMaterialDesc.unionTags & SMOOTHNESS_IS_MAP) == 0) {
			m->matPrms.roughness = 1.0f - standardMaterialDesc.smoothnessValue;
			m->matPrms.hasSmoothnessMap = false;
		}

	}

	return MaterialCacheHandle_t{ (std::uint16_t)matIndex, NO_UPDATE_MAT };
}

//TODO:implement material dealloc
void ResourceCache::DeallocMaterial(MaterialCacheHandle_t& matCacheHnd)
{
	//TODO: what about deallocating the gpu resources (e.g. texture, ...)
	freeMaterialCacheIndices.push_back(matCacheHnd.index);
	Material& mat = materialCache.at(matCacheHnd.index);
	mat.model.freeShader();
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
