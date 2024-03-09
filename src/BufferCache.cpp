#include "BufferCache.h"
#include "RenderHardwareInterface.h"
#include "Model.h"

BufferCache bufferCache;

BufferCache::BufferCache()
{
	staticCache.vertexBuffers.resize(MIN_VERTEX_CACHE_SIZE);
	staticCache.indexBuffers.resize(MIN_INDEX_CACHE_SIZE);
	staticCache.topIndexBuffer = 0;
	staticCache.topVertexBuffer = 0;
}

BufferCache::~BufferCache()
{
}

BufferCacheHandle_t BufferCache::AllocStaticGeom(const SubmeshHandle_t sHnd, const std::uint16_t* geomIndStart, const Vertex* geomVertStart)
{
	ID3D11Buffer* iBuffer = nullptr;

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.ByteWidth = sHnd.numIndexes * sizeof(std::uint16_t);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	
	D3D11_SUBRESOURCE_DATA bufferData;
	ZeroMemory(&bufferData, sizeof(bufferData));
	bufferData.pSysMem = geomIndStart;

	/*hr = */rhi.device.device->CreateBuffer(&bufferDesc, &bufferData, &iBuffer);
	staticCache.indexBuffers.at(staticCache.topIndexBuffer).Init(iBuffer, sHnd.numIndexes);
	staticCache.topIndexBuffer += 1;

	ID3D11Buffer* vBuffer = nullptr;

	bufferDesc.ByteWidth = sHnd.numVertices * sizeof(Vertex);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	bufferData.pSysMem = geomVertStart;

	/*hr = */rhi.device.device->CreateBuffer(&bufferDesc, &bufferData, &vBuffer);
	staticCache.vertexBuffers.at(staticCache.topVertexBuffer).Init(vBuffer, sHnd.numVertices);
	staticCache.topVertexBuffer += 1;

	//TODO: create better implementation; this is a naive solution
	BufferCacheHandle_t handle = BufferCacheHandle_t{ ((std::uint32_t)staticCache.topVertexBuffer) - 1, 1 };

	return handle;
}

bool BufferCache::isCached(const SubmeshHandle_t sHnd)
{
	//TODO: make better implementation; this should be only temporary
	return sHnd.baseVertex < staticCache.topVertexBuffer;
}

VertexBuffer BufferCache::GetVertexBuffer(BufferCacheHandle_t bufHnd)
{
	if (!((bool)bufHnd.isCached)) {
		OutputDebugString("ERROR :: the buffer handle specified in BufferCache::GetVertexBuffer is not valid\n");
		return VertexBuffer();
	}
	
	return staticCache.vertexBuffers.at(bufHnd.value);
}

IndexBuffer BufferCache::GetIndexBuffer(BufferCacheHandle_t bufHnd)
{
	if (!((bool)bufHnd.isCached)) {
		OutputDebugString("ERROR :: the buffer handle specified in BufferCache::GetIndexBuffer is not valid\n");
		return IndexBuffer();
	}

	return staticCache.indexBuffers.at(bufHnd.value);
}
