#include "BufferCache.h"

BufferCache::BufferCache()
{
	staticCache.vertexBuffer.resize(MIN_VERTEX_CACHE_SIZE);
	staticCache.indexBuffer.resize(MIN_INDEX_CACHE_SIZE);
	staticCache.topIndexBuffer = 0;
	staticCache.topVertexBuffer = 0;
}

BufferCache::~BufferCache()
{
}
