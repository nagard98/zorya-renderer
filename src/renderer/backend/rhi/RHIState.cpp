#include "RHIState.h"

namespace zorya
{
	constexpr RHI_State RHI_DEFAULT_STATE()
	{
		constexpr RHI_State cull_mode = RHI_RS_SET_CULL_BACK_RET(0);
		constexpr RHI_State fill_mode = RHI_RS_SET_SOLID_FILL_RET(0);
		constexpr RHI_State antialiasing = RHI_RS_DISABLE_ANTIALIAS_RET(0);
		constexpr RHI_State fcw = RHI_RS_SET_FCW_RET(0);
		constexpr RHI_State multisample = RHI_RS_DISABLE_MULTISAMPLE_RET(0);
		constexpr RHI_State scissor = RHI_RS_DISABLE_SCISSOR_RET(0);

		constexpr RHI_State depth_enable = RHI_OM_DS_ENABLE_DEPTH_RET(0);
		constexpr RHI_State depth_write_enable = RHI_OM_DS_ENABLE_DEPTH_WRITE_RET(0);

		return (cull_mode | fill_mode | antialiasing | fcw | multisample | scissor | depth_enable | depth_write_enable);
	}

	RHI_State rhi_state = RHI_DEFAULT_STATE();
}