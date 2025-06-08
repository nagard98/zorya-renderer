#ifndef RENDER_DEVICE_H_
#define RENDER_DEVICE_H_

#include <iostream>
#include <cstdint>
#include <d3d11_1.h>
#include <wrl/client.h>
#include <vector>
#include <unordered_map>

#include "Platform.h"

#include <renderer/backend/ConstantBuffer.h>
#include <renderer/backend/rhi/RenderDeviceHandles.h>

namespace zorya
{
	struct PSO_Desc;
	struct Pipeline_State_Object;
	struct Pixel_Shader_Handle;
	struct Vertex_Shader_Handle;
	struct Shader_Bytecode;
	
	namespace wrl = Microsoft::WRL;

	//TODO: correct types for all these structs
	struct Result_Code
	{
		HRESULT value;
	};

	enum class Texture_Format : uint8_t
	{
		UNKNOWN,
		R8G8B8A8_TYPELESS = DXGI_FORMAT_R8G8B8A8_TYPELESS,
		R8G8B8A8_UNORM = DXGI_FORMAT_R8G8B8A8_UNORM,
		R8G8B8A8_UNORM_SRGB = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		R11G11B10_FLOAT = DXGI_FORMAT_R11G11B10_FLOAT,
		D32_FLOAT = DXGI_FORMAT_D32_FLOAT,
		R32_FLOAT = DXGI_FORMAT_R32_FLOAT,
		R32_TYPELESS = DXGI_FORMAT_R32_TYPELESS,
		D24_UNORM_S8_UINT = DXGI_FORMAT_D24_UNORM_S8_UINT,
		R24_UNORM_X8_TYPELESS = DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
		R24G8_TYPELESS = DXGI_FORMAT_R24G8_TYPELESS,
		R16G16_UNORM = DXGI_FORMAT_R16G16_UNORM,
		R16G16_TYPELESS = DXGI_FORMAT_R16G16_TYPELESS,
		R32G32B32A32_FLOAT = DXGI_FORMAT_R32G32B32A32_FLOAT
	};

	enum class CPU_Access_Flags : uint8_t
	{
		NONE = 0,
		WRITE = 1,
		READ = 2
	};

	enum class Resource_Misc_Flags : uint16_t
	{
		NONE = 0,
		GENERATE_MIPS = D3D11_RESOURCE_MISC_GENERATE_MIPS,
		TEXTURE_CUBE = D3D11_RESOURCE_MISC_TEXTURECUBE
	};

	enum class Resource_Bind_Flags : uint8_t
	{
		NONE = 0,
		RENDER_TARGET = D3D11_BIND_RENDER_TARGET,
		DEPTH_STENCIL = D3D11_BIND_DEPTH_STENCIL,
		SHADER_RESOURCE = D3D11_BIND_SHADER_RESOURCE,
		CONSTANT_BUFFER = D3D11_BIND_CONSTANT_BUFFER,
		INDEX_BUFFER = D3D11_BIND_INDEX_BUFFER,
		VERTEX_BUFFER = D3D11_BIND_VERTEX_BUFFER,
		UNORDERED_ACCESS = D3D11_BIND_UNORDERED_ACCESS,
	};

	inline Resource_Bind_Flags operator|(Resource_Bind_Flags lhs, Resource_Bind_Flags rhs)
	{
		using T = std::underlying_type_t<Resource_Bind_Flags>;
		return static_cast<Resource_Bind_Flags>(static_cast<T>(lhs) | static_cast<T>(rhs));
	}

	inline Resource_Bind_Flags operator&(Resource_Bind_Flags lhs, Resource_Bind_Flags rhs)
	{
		using T = std::underlying_type_t<Resource_Bind_Flags>;
		return static_cast<Resource_Bind_Flags>(static_cast<T>(lhs) & static_cast<T>(rhs));
	}

	inline Resource_Bind_Flags& operator|=(Resource_Bind_Flags& lhs, Resource_Bind_Flags rhs)
	{
		lhs = lhs | rhs;
		return lhs;
	}

	inline Resource_Bind_Flags& operator&=(Resource_Bind_Flags& lhs, Resource_Bind_Flags rhs)
	{
		lhs = lhs & rhs;
		return lhs;
	}


	enum class Resource_Usage : uint8_t
	{
		DEFAULT = D3D11_USAGE::D3D11_USAGE_DEFAULT,
		DYNAMIC = D3D11_USAGE::D3D11_USAGE_DYNAMIC,
		IMMUTABLE = D3D11_USAGE::D3D11_USAGE_IMMUTABLE,
		STAGING = D3D11_USAGE::D3D11_USAGE_STAGING,
	};

	enum class Comparison_Func
	{
		NEVER = D3D11_COMPARISON_NEVER,
		LESS = D3D11_COMPARISON_LESS,
		EQUAL = D3D11_COMPARISON_EQUAL,
		LESS_EQUAL = D3D11_COMPARISON_LESS_EQUAL,
		GREATER = D3D11_COMPARISON_GREATER,
		NOT_EQUAL = D3D11_COMPARISON_NOT_EQUAL,
		GREATER_EQUAL = D3D11_COMPARISON_GREATER_EQUAL,
		ALWAYS = D3D11_COMPARISON_ALWAYS
	};

	enum class Stencil_Op : uint8_t
	{
		KEEP = D3D11_STENCIL_OP_KEEP,
		ZERO = D3D11_STENCIL_OP_ZERO,
		REPLACE = D3D11_STENCIL_OP_REPLACE,
		INVERT = D3D11_STENCIL_OP_INVERT,
		INCR = D3D11_STENCIL_OP_INCR,
		DECR = D3D11_STENCIL_OP_DECR
	};

	struct Stencil_Op_Desc
	{
		Stencil_Op stencil_fail_op;
		Stencil_Op stencil_pass_depth_fail_op;
		Stencil_Op stencil_depth_pass_op;
		Comparison_Func stencil_test_func;
	};

	struct Depth_Stencil_State_Desc
	{
		static Depth_Stencil_State_Desc create();

		bool depth_enable;
		uint8_t depth_write_mask;
		Comparison_Func depth_test_func;

		bool stencil_enable;
		uint8_t stencil_read_mask;
		uint8_t stencil_write_mask;
		
		Stencil_Op_Desc front_face;
		Stencil_Op_Desc back_face;
	};
	
	struct Buffer_Desc
	{
		static Buffer_Desc create();

		uint32_t byte_width;
		Resource_Usage usage;
		Resource_Bind_Flags bind_flags;
		Resource_Misc_Flags misc_flags;
		CPU_Access_Flags access_flags;
		uint32_t byte_stride_structured_buff;
	};

	struct Texture_2D_Desc
	{
		static Texture_2D_Desc create();

		Resource_Usage resource_usage;
		Resource_Bind_Flags bind_flags;
		Resource_Misc_Flags misc_flags;
		Texture_Format format;
		float width;
		float height;
		uint32_t array_size;
		uint8_t mip_levels;
		uint8_t sample_count;
		uint8_t sample_quality;
	};

	enum class Fill_Mode : uint8_t
	{
		WIREFRAME = D3D11_FILL_MODE::D3D11_FILL_WIREFRAME,
		SOLID = D3D11_FILL_MODE::D3D11_FILL_SOLID
	};

	enum class Cull_Mode : uint8_t
	{
		NONE = D3D11_CULL_MODE::D3D11_CULL_NONE,
		FRONT = D3D11_CULL_MODE::D3D11_CULL_FRONT,
		BACK = D3D11_CULL_MODE::D3D11_CULL_BACK
	};

	struct Rasterizer_State_Desc
	{
		static Rasterizer_State_Desc create();

		Fill_Mode fill_mode;
		Cull_Mode cull_mode;
		bool is_front_counter_clock_wise;
		int32_t depth_bias;
		float depth_bias_clamp;
		float slope_depth_bias;
		bool is_depth_clip_enabled;
		bool is_scissor_culling_enabled;
		bool is_multisample_enabled;
		bool is_antialiased_line_enabled;
	};

	template <class T>
	class Render_Device
	{

	public:
		Result_Code create_tex_2d()
		{
			return impl().create_tex_2d();
		}

	private:
		T& impl()
		{
			return *static_cast<T*>(this);
		}

	};

}
#endif