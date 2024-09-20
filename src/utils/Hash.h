#ifndef HASH_H_
#define HASH_H_

#include <cstdint>
#include <string>

namespace zorya
{
	uint32_t hash_str_uint32(const std::string& str);
}

#endif