#include <cstdint>
#include <vector>
#include <d3d11_1.h>

#include "BufferObject.h"

constexpr int MIN_VERTEX_CACHE_SIZE = 32;
constexpr int MIN_INDEX_CACHE_SIZE = 32;

typedef std::uint64_t BufferCacheHandle_t;

struct Vertex;

struct GeomCache 
{
	std::vector<VertexBuffer> vertexBuffer;
	std::vector<IndexBuffer> indexBuffer;
	int topVertexBuffer;
	int topIndexBuffer;
};

class BufferCache 
{
public:
	BufferCache();
	~BufferCache();

	BufferCacheHandle_t AllocStaticGeom(const std::vector<std::uint16_t>& indices, const std::vector<Vertex>& vertices );

	GeomCache staticCache;

private:
	
};

extern BufferCache bufferCache;