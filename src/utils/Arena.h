#ifndef ARENA_H_
#define ARENA_H_

#include "Platform.h"

#include <cstdint>

namespace zorya
{
	struct Arena
	{
		Arena(uint64_t buff_size);

		void* alloc(uint64_t bytes, uint64_t allign);
		void dealloc(void* p);

		void clear();

		byte* buff;
		uint64_t size;
		uint64_t offset;
	};

}
#endif