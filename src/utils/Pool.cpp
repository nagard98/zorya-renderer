#include "Pool.h"

#include <cstdlib>
#include <cstring>

zorya::Pool::Pool(uint64_t buff_len, uint64_t chunk_size, uint64_t chunk_alignment)
{
	buff = (byte*)_aligned_malloc(chunk_size * buff_len, chunk_alignment);
	zassert(buff != nullptr);

	this->buff_len = buff_len;
	this->chunk_size = chunk_size;
	this->first_free = nullptr;

	clear();	
}

void* zorya::Pool::alloc()
{
	auto alloced_chunk = first_free;
	zassert(alloced_chunk != nullptr);

	first_free = first_free->next_free;
	
	return memset(alloced_chunk, 0, chunk_size);
}

void zorya::Pool::dealloc(void* ptr)
{
	zassert(ptr != nullptr);
	auto node = static_cast<Pool_Free_Node*>(ptr);
	node->next_free = first_free;
	first_free = node;
}

void zorya::Pool::clear()
{
	for (int chunk_index = 0; chunk_index < buff_len; chunk_index++)
	{
		void* current_chunk = &buff[chunk_size * chunk_index];
		auto node = static_cast<Pool_Free_Node*>(current_chunk);
		node->next_free = first_free;
		first_free = node;
	}
}
