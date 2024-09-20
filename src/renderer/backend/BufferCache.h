#ifndef BUFFER_CACHE_H_
#define BUFFER_CACHE_H_

#include <cstdint>
#include <vector>
#include <d3d11_1.h>

#include "renderer/backend/rhi/BufferObject.h"

#include "renderer/frontend/Model.h"

namespace zorya
{
	constexpr int MIN_VERTEX_CACHE_SIZE = 4096;
	constexpr int MIN_INDEX_CACHE_SIZE = 4096;

	//TODO: use bitwise operation to reduce size, instead of separate variables
	struct Buffer_Handle_t
	{
		uint32_t index;
		uint8_t is_cached;
	};

	struct Vertex;

	struct Geom_Cache
	{
		std::vector<Vertex_Buffer> vertex_buffers;
		std::vector<Index_Buffer> index_buffers;
		uint16_t top_vertex_buffer;
		uint16_t top_index_buffer;
	};

	class Buffer_Cache
	{
	public:
		Buffer_Cache();
		~Buffer_Cache();

		void release_all_resources();

		Buffer_Handle_t alloc_static_geom(const Submesh_Handle_t hnd_submesh,
			const uint16_t* itInd,
			const Vertex* itVer);

		void dealloc_static_geom(Buffer_Handle_t hnd_buffer_gpu_resource);

		bool is_cached(const Submesh_Handle_t hnd_submesh);

		Vertex_Buffer get_vertex_buffer(Buffer_Handle_t hnd_buffer_gpu_resource);
		Index_Buffer get_index_buffer(Buffer_Handle_t hnd_buffer_gpu_resource);

		Geom_Cache m_static_cache;
	};

	extern Buffer_Cache buffer_cache;
}

#endif