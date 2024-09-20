#include "Platform.h"

#include "Windows.h"

namespace zorya
{
	void zassert(bool condition)
	{
		if (condition == false)
		{
			DebugBreak();
		}
	}

	constexpr bool is_power_of_2(uintptr_t ptr)
	{
		return (ptr & (ptr - 1)) == 0;
	}

	uintptr_t first_aligned_addr(uintptr_t ptr, uint64_t align)
	{
		zassert(is_power_of_2(align));

		uintptr_t a, modulo;
		a = (uintptr_t)align;

		modulo = ptr & (a - 1);
		if (modulo != 0) ptr += a - modulo;
		
		return ptr;
	}


}