#include "renderer/backend/BufferCache.h"
#include "renderer/backend/rhi/RenderHardwareInterface.h"

#include "renderer/frontend/Model.h"

namespace zorya
{
	Buffer_Cache buffer_cache;

	Buffer_Cache::Buffer_Cache()
	{
		m_static_cache.vertex_buffers.resize(MIN_VERTEX_CACHE_SIZE);
		m_static_cache.index_buffers.resize(MIN_INDEX_CACHE_SIZE);
		m_static_cache.top_index_buffer = 0;
		m_static_cache.top_vertex_buffer = 0;
	}

	Buffer_Cache::~Buffer_Cache()
	{}

	void Buffer_Cache::release_all_resources()
	{
		m_static_cache.vertex_buffers.clear();
		m_static_cache.top_vertex_buffer = 0;

		m_static_cache.index_buffers.clear();
		m_static_cache.top_index_buffer = 0;
	}

	Buffer_Cache_Handle_t Buffer_Cache::alloc_static_geom(const Submesh_Handle_t hnd_submesh, const uint16_t* geom_index_start, const Vertex* geom_vertex_start)
	{
		ID3D11Buffer* index_buffer = nullptr;

		D3D11_BUFFER_DESC buffer_desc;
		ZeroMemory(&buffer_desc, sizeof(buffer_desc));
		buffer_desc.ByteWidth = hnd_submesh.num_indices * sizeof(uint16_t);
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

		D3D11_SUBRESOURCE_DATA buffer_data;
		ZeroMemory(&buffer_data, sizeof(buffer_data));
		buffer_data.pSysMem = geom_index_start;

		/*hr = */rhi.m_device.m_device->CreateBuffer(&buffer_desc, &buffer_data, &index_buffer);
		m_static_cache.index_buffers.at(m_static_cache.top_index_buffer).init(index_buffer, hnd_submesh.num_indices);
		m_static_cache.top_index_buffer += 1;

		ID3D11Buffer* vertex_buffer = nullptr;

		buffer_desc.ByteWidth = hnd_submesh.num_vertices * sizeof(Vertex);
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		buffer_data.pSysMem = geom_vertex_start;

		/*hr = */rhi.m_device.m_device->CreateBuffer(&buffer_desc, &buffer_data, &vertex_buffer);
		m_static_cache.vertex_buffers.at(m_static_cache.top_vertex_buffer).init(vertex_buffer, hnd_submesh.num_vertices);
		m_static_cache.top_vertex_buffer += 1;

		//TODO: create better implementation; this is a naive solution
		Buffer_Cache_Handle_t handle = Buffer_Cache_Handle_t{ ((uint32_t)m_static_cache.top_vertex_buffer) - 1, 1 };

		return handle;
	}

	//TODO: implement deallocation static geometry from buffer cache
	void Buffer_Cache::dealloc_static_geom(Buffer_Cache_Handle_t hnd_buffer_cache)
	{}

	bool Buffer_Cache::is_cached(const Submesh_Handle_t hnd_submesh)
	{
		//TODO: make better implementation; this should be only temporary
		return hnd_submesh.base_vertex < m_static_cache.top_vertex_buffer;
	}

	Vertex_Buffer Buffer_Cache::get_vertex_buffer(Buffer_Cache_Handle_t hnd_buffer_cache)
	{
		if (!((bool)hnd_buffer_cache.is_cached))
		{
			OutputDebugString("ERROR :: the buffer handle specified in BufferCache::GetVertexBuffer is not valid\n");
			return Vertex_Buffer();
		}

		return m_static_cache.vertex_buffers.at(hnd_buffer_cache.value);
	}

	Index_Buffer Buffer_Cache::get_index_buffer(Buffer_Cache_Handle_t hnd_buffer_cache)
	{
		if (!((bool)hnd_buffer_cache.is_cached))
		{
			OutputDebugString("ERROR :: the buffer handle specified in BufferCache::GetIndexBuffer is not valid\n");
			return Index_Buffer();
		}

		return m_static_cache.index_buffers.at(hnd_buffer_cache.value);
	}
}
