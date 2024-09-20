#include "Hash.h"

#include <cstdint>
#include <string>

namespace zorya
{
    //TODO: move somewhere else and use other hashing function; this is temporary
    uint32_t hash_str_uint32(const std::string& str)
    {
        uint32_t hash = 0x811c9dc5;
        uint32_t prime = 0x1000193;

        for (int i = 0; i < str.size(); ++i)
        {
            uint8_t value = str[i];
            hash = hash ^ value;
            hash *= prime;
        }

        return hash;

    }
}