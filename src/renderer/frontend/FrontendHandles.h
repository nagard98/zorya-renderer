#ifndef FRONTEND_HANDLES_H_
#define FRONTEND_HANDLES_H_

#include <Platform.h>

namespace zorya
{
	typedef u8 MatUpdateFlags_;

	struct Material_Cache_Handle_t
	{
		u16 index;
		MatUpdateFlags_ is_cached;
	};

	struct Diffusion_Profile_Handle
	{
		u16 index;
	};
}

#endif