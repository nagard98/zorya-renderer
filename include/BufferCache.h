#ifndef BUFFER_CACHE_H_
#define BUFFER_CACHE_H_

#include <cstdint>
#include <vector>
#include <d3d11_1.h>

#include "BufferObject.h"
#include "Model.h"

constexpr int MIN_VERTEX_CACHE_SIZE = 4096;
constexpr int MIN_INDEX_CACHE_SIZE = 4096;

//TODO: use bitwise operation to reduce size, instead of separate variables
struct BufferCacheHandle_t {
	std::uint32_t value;
	std::uint8_t isCached;

};

struct Vertex;

struct GeomCache 
{
	std::vector<VertexBuffer> vertexBuffers;
	std::vector<IndexBuffer> indexBuffers;
	std::uint16_t topVertexBuffer;
	std::uint16_t topIndexBuffer;
};

class BufferCache 
{
public:
	BufferCache();
	~BufferCache();

	BufferCacheHandle_t AllocStaticGeom(const SubmeshHandle_t sHnd,
		const std::uint16_t* itInd,
		const Vertex* itVer);

	bool isCached(const SubmeshHandle_t sHnd);

	VertexBuffer GetVertexBuffer(BufferCacheHandle_t bufHnd);
	IndexBuffer GetIndexBuffer(BufferCacheHandle_t bufHnd);

	GeomCache staticCache;	
};

extern BufferCache bufferCache;

#endif