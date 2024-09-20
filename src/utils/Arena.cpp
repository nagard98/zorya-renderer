#include "Arena.h"

#include <cstdlib>
#include <cstdint>
#include <cstring>

zorya::Arena::Arena(uint64_t buff_size)
{
    buff = (byte*)malloc(buff_size);
    zassert(buff != nullptr);

    size = buff_size;
    offset = 0;
}

void* zorya::Arena::alloc(uint64_t bytes, uint64_t align)
{

    uintptr_t align_addr = first_aligned_addr((uintptr_t)(buff + offset), align);
    uintptr_t current_occupied_space = align_addr - reinterpret_cast<uintptr_t>(buff);
    
    offset = current_occupied_space + bytes;

    zassert(offset <= size);

    return memset(reinterpret_cast<void*>(align_addr), 0, bytes);
}

void zorya::Arena::dealloc(void* p)
{
    //NULL-OP
}

void zorya::Arena::clear()
{
    offset = 0;
}


