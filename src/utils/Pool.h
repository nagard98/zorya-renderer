#ifndef POOL_H_
#define POOL_H_

#include "Platform.h"

#include <cstdint>

namespace zorya
{

	struct Pool_Free_Node
	{
		Pool_Free_Node* next_free;
	};

	struct Pool
	{
		Pool(uint64_t buff_len, uint64_t chunk_size, uint64_t chunk_alignment);

		byte* buff;
		uint64_t buff_len;
		uint64_t chunk_size;
		Pool_Free_Node* first_free;

		void* alloc();
		void dealloc(void* ptr);

		void clear();
	};
}

#endif