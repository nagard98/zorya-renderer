#ifndef CONSTANT_BUFFER_H_
#define CONSTANT_BUFFER_H_

#include <d3d11_1.h>
#include <cstdint>

namespace zorya
{
	struct Constant_Buffer
	{
		ID3D11Buffer* buffer;
		const char* constant_buffer_name;
	};
}

#endif