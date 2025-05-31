#ifndef RENDER_DEVICE_HANDLES_H_
#define RENDER_DEVICE_HANDLES_H_

#include <cstdint>
#include "Platform.h"

namespace zorya
{
	enum class Render_Resource_Type : uint8_t
	{
		Undefined,
		Texture,
		Constant_Buffer,
		PSO,
		SRV,
		RTV,
		DSV,
		Depth_Stencil_State,
		Rasterizer_State,
		Blend_State
	};

	struct Render_Resource_Handle
	{
		using Key = uint32_t;

		Render_Resource_Handle() = default;

		template<typename T>
		Render_Resource_Handle(const T resource_handle)
		{
			type = T::type_id;
			index = resource_handle.index;
		}

		Key index{ 0 };
		Render_Resource_Type type{ Render_Resource_Type::Undefined };
	};

	struct PSO_Handle
	{
		static constexpr Render_Resource_Type type_id{ Render_Resource_Type::PSO };
		
		PSO_Handle() = default;
		PSO_Handle(const Render_Resource_Handle::Key key) : index(key) {};
		PSO_Handle(const Render_Resource_Handle hnd_resource)
		{
			zassert(hnd_resource.type == Render_Resource_Type::PSO);
			index = hnd_resource.index;
		}

		Render_Resource_Handle::Key index{ 0 };
	};

	struct Constant_Buffer_Handle
	{
		static constexpr Render_Resource_Type type_id{ Render_Resource_Type::Constant_Buffer };

		Constant_Buffer_Handle() = default;
		Constant_Buffer_Handle(const Render_Resource_Handle::Key key) : index(key) {};
		Constant_Buffer_Handle(const Render_Resource_Handle hnd_resource)
		{
			zassert(hnd_resource.type == Render_Resource_Type::Constant_Buffer);
			index = hnd_resource.index;
		}

		Render_Resource_Handle::Key index{ 0 };
	};

	struct Render_Texture_Handle
	{
		static constexpr Render_Resource_Type type_id{ Render_Resource_Type::Texture };

		Render_Texture_Handle() = default;
		Render_Texture_Handle(const Render_Resource_Handle::Key key) : index(key) {};
		Render_Texture_Handle(const Render_Resource_Handle hnd_resource)
		{
			zassert(hnd_resource.type == Render_Resource_Type::Texture);
			index = hnd_resource.index;
		}

		Render_Resource_Handle::Key index{0};
	};

	struct Render_SRV_Handle
	{
		static constexpr Render_Resource_Type type_id{ Render_Resource_Type::SRV };
		
		Render_SRV_Handle() = default;
		Render_SRV_Handle(const Render_Resource_Handle::Key key) : index(key) {};
		Render_SRV_Handle(const Render_Resource_Handle hnd_resource)
		{
			zassert(hnd_resource.type == Render_Resource_Type::SRV);
			index = hnd_resource.index;
		}

		Render_Resource_Handle::Key index{ 0 };
	};

	struct Render_RTV_Handle
	{
		static constexpr Render_Resource_Type type_id{ Render_Resource_Type::RTV };

		Render_RTV_Handle() = default;
		Render_RTV_Handle(const Render_Resource_Handle::Key key) : index(key) {};
		Render_RTV_Handle(const Render_Resource_Handle hnd_resource)
		{
			zassert(hnd_resource.type == Render_Resource_Type::RTV);
			index = hnd_resource.index;
		}

		Render_Resource_Handle::Key index{ 0 };
	};

	struct Render_DSV_Handle
	{
		static constexpr Render_Resource_Type type_id{ Render_Resource_Type::DSV };

		Render_DSV_Handle() = default;
		Render_DSV_Handle(const Render_Resource_Handle::Key key) : index(key) {};
		Render_DSV_Handle(const Render_Resource_Handle hnd_resource)
		{
			zassert(hnd_resource.type == Render_Resource_Type::DSV);
			index = hnd_resource.index;
		}

		Render_Resource_Handle::Key index{ 0 };
	};

	struct DS_State_Handle
	{
		static constexpr Render_Resource_Type type_id{ Render_Resource_Type::Depth_Stencil_State };
		
		DS_State_Handle() = default;
		DS_State_Handle(const Render_Resource_Handle::Key key) : index(key) {};
		DS_State_Handle(const Render_Resource_Handle hnd_resource)
		{
			zassert(hnd_resource.type == Render_Resource_Type::Depth_Stencil_State);
			index = hnd_resource.index;
		}

		Render_Resource_Handle::Key index{ 0 };
	};

	struct RS_State_Handle
	{
		static constexpr Render_Resource_Type type_id{ Render_Resource_Type::Rasterizer_State };
		
		RS_State_Handle() = default;
		RS_State_Handle(const Render_Resource_Handle::Key key) : index(key) {};
		RS_State_Handle(const Render_Resource_Handle hnd_resource)
		{
			zassert(hnd_resource.type == Render_Resource_Type::Rasterizer_State);
			index = hnd_resource.index;
		}

		Render_Resource_Handle::Key index{ 0 };
	};

	struct BL_State_Handle
	{
		static constexpr Render_Resource_Type type_id{ Render_Resource_Type::Blend_State};
		
		BL_State_Handle() = default;
		BL_State_Handle(const Render_Resource_Handle::Key key) : index(key) {};
		BL_State_Handle(const Render_Resource_Handle hnd_resource)
		{
			zassert(hnd_resource.type == Render_Resource_Type::Blend_State);
			index = hnd_resource.index;
		}
		
		Render_Resource_Handle::Key index{ 0 };
	};
}

#endif