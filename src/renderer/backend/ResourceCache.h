#ifndef RESOURCE_CACHE_H_
#define RESOURCE_CACHE_H_

#include <vector>
#include <cstdint>

#include "renderer/frontend/Material.h"

#include "reflection/Reflection.h"

struct MaterialCacheHandle_t {
	std::uint16_t index;
	MatUpdateFlags_ isCached;
};

class ResourceCache {
public:
	ResourceCache();
	~ResourceCache();

	void ReleaseAllResources();

	MaterialCacheHandle_t AllocMaterial(const ReflectionBase *matDesc, MaterialCacheHandle_t& updateOpts);
	void DeallocMaterial(MaterialCacheHandle_t& matCacheHnd);
	void UpdateMaterialSmoothness(const MaterialCacheHandle_t matHnd, float smoothness );
	void UpdateMaterialAlbedoMap(const MaterialCacheHandle_t matHnd, const wchar_t* albedoMapPath);

	std::vector<Material> materialCache;

private:
	std::vector<int> freeMaterialCacheIndices;

};

extern ResourceCache resourceCache;

#endif