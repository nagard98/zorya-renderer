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

	std::vector<Material> materialCache;
};

extern ResourceCache resourceCache;

#endif