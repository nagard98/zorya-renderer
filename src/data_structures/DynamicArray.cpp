#include "DynamicArray.h"

zorya::Dynamic_Array_Storage::Dynamic_Array_Storage(uint64_t _element_size, uint64_t _alignment, Arena* _arena)
{
	element_size = _element_size;
	alignment = _alignment;
	arena = _arena;

	capacity = 1000;
	size = 0;
	data = arena->alloc(capacity, alignment);
}
