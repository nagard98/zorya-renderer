#ifndef CONSTANT_BUFFER_H_
#define CONSTANT_BUFFER_H_

#include <d3d11_1.h>
#include <cstdint>

struct ConstantBuffer {
	ID3D11Buffer* buffer;
	const char* constantBufferName;
};

template <typename T>
struct ConstantBufferHandle {
	std::uint64_t index;
};

#endif