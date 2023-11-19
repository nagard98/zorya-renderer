#ifndef RESOURCE_CACHE_H_
#define RESOURCE_CACHE_H_

#include <vector>
#include <cstdint>

#include "Material.h"

struct MaterialCacheHandle_t {
	std::uint16_t index;
	std::uint8_t isCached;
};

class ResourceCache {
public:
	ResourceCache();
	~ResourceCache();

	MaterialCacheHandle_t AllocMaterial(const MaterialDesc &matDesc);
	void UpdateMaterialSmoothness(const MaterialCacheHandle_t matHnd, float smoothness );
	void UpdateMaterialAlbedoMap(const MaterialCacheHandle_t matHnd, const wchar_t* albedoMapPath);

	std::vector<Material> materialCache;
};

extern ResourceCache resourceCache;

#endif