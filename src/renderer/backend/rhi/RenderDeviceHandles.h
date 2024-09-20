#ifndef RENDER_DEVICE_HANDLES_H_
#define RENDER_DEVICE_HANDLES_H_

#include <cstdint>

namespace zorya
{
	struct Render_Resource_Handle
	{
		uint32_t index;
	};

	struct PSO_Handle
	{
		uint32_t index;
	};

	struct Constant_Buffer_Handle
	{
		uint32_t index;
	};

	struct Render_Texture_Handle
	{
		uint64_t index;
	};

	struct Render_SRV_Handle
	{
		uint64_t index;
	};

	struct Render_RTV_Handle
	{
		uint64_t index;
	};

	struct Render_DSV_Handle
	{
		uint64_t index;
	};

	struct DS_State_Handle
	{
		uint32_t index;
	};

	struct RS_State_Handle
	{
		uint32_t index;
	};

	struct BL_State_Handle
	{
		uint32_t index;
	};
}

#endif