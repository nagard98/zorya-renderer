#ifndef PLATFORM_H_
#define PLATFORM_H_

#include <cstdint>
#include "DirectXMath.h"

namespace zorya
{
	#define RETURN_IF_FAILED(h_result) { if(FAILED(h_result)) return h_result; }
	#define RETURN_IF_FAILED2(hr_var, op_code) { hr_var = op_code; if(FAILED(hr_var)) return hr_var; }
	#define RETURN_IF_FAILED_ZRY(z_result) { if(FAILED(z_result.value)) return z_result; }

	using byte = unsigned char;

	using float2 = DirectX::XMFLOAT2;
	using float3 = DirectX::XMFLOAT3;
	using float4 = DirectX::XMFLOAT4;

	using u8 = uint8_t;
	using s8 = int8_t;
	using u16 = uint16_t;
	using s16 = int16_t;
	using u32 = uint32_t;
	using s32 = int32_t;
	using u64 = uint64_t;
	using s64 = int64_t;

	using Guid = uint64_t;

	void zassert(bool condition);

	constexpr bool is_power_of_2(uintptr_t ptr);

	uintptr_t first_aligned_addr(uintptr_t ptr, uint64_t align);
}

#endif // !PLATFORM_H_
