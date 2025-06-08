#include "RenderDevice.h"

namespace zorya
{
	Depth_Stencil_State_Desc Depth_Stencil_State_Desc::create()
	{
		Depth_Stencil_State_Desc desc;
		desc.depth_enable = true;
		desc.depth_write_mask = 1;
		desc.depth_test_func = Comparison_Func::LESS;
		desc.stencil_enable = false;
		desc.stencil_read_mask = 0xff;
		desc.stencil_write_mask = 0xff;
		desc.front_face.stencil_fail_op = Stencil_Op::KEEP;
		desc.front_face.stencil_depth_pass_op = Stencil_Op::KEEP;
		desc.front_face.stencil_pass_depth_fail_op = Stencil_Op::KEEP;
		desc.front_face.stencil_test_func = Comparison_Func::ALWAYS;

		desc.back_face.stencil_fail_op = Stencil_Op::KEEP;
		desc.back_face.stencil_depth_pass_op = Stencil_Op::KEEP;
		desc.back_face.stencil_pass_depth_fail_op = Stencil_Op::KEEP;
		desc.back_face.stencil_test_func = Comparison_Func::ALWAYS;

		return desc;
	}

	Buffer_Desc Buffer_Desc::create()
	{
		Buffer_Desc desc;
		desc.byte_width = 0;
		desc.usage = Resource_Usage::DEFAULT;
		desc.bind_flags = Resource_Bind_Flags::CONSTANT_BUFFER;
		desc.misc_flags = Resource_Misc_Flags::NONE;
		desc.access_flags = CPU_Access_Flags::NONE;
		desc.byte_stride_structured_buff = 0;

		return desc;
	}

	Rasterizer_State_Desc Rasterizer_State_Desc::create()
	{
		Rasterizer_State_Desc desc;
		desc.cull_mode = Cull_Mode::BACK;
		desc.fill_mode = Fill_Mode::SOLID;
		desc.is_front_counter_clock_wise = false;
		desc.depth_bias = 0;
		desc.depth_bias_clamp = 0.0f;
		desc.slope_depth_bias = 0.0f;
		desc.is_depth_clip_enabled = true;
		desc.is_scissor_culling_enabled = false;
		desc.is_multisample_enabled = false;
		desc.is_antialiased_line_enabled = false;

		return desc;
	}

	Texture_2D_Desc Texture_2D_Desc::create()
	{
		Texture_2D_Desc desc;
		desc.misc_flags = Resource_Misc_Flags::NONE;
		desc.array_size = 1;
		desc.mip_levels = 1;
		desc.sample_count = 1;
		desc.sample_quality = 0;

		return desc;
	}

}
