#ifndef MATERIAL_H_
#define MATERIAL_H_

#include <DirectXMath.h>
#include <string>
#include <cstdint>
#include <d3d11.h>

#include "Shader.h"
#include "reflection/ReflectionAnnotation.h"

namespace zorya
{
	namespace dx = DirectX;

	enum class SSS_MODEL : uint32_t
	{
		NONE,
		JIMENEZ_GAUSS,
		JIMENEZ_SEPARABLE,
		GOLUBEV
	};

	struct Jimenez_SSS_Model
	{
		PROPERTY()
			dx::XMFLOAT4 subsurface_albedo;

		PROPERTY()
			dx::XMFLOAT4 mean_free_path_color;

		PROPERTY()
			float mean_free_path_distance;

		PROPERTY()
			float scale;

		PROPERTY()
			uint32_t num_samples;
	};

	struct Multi_Option
	{
		uint8_t selected_option;

		union
		{
			Jimenez_SSS_Model sss_model;
		};
	};


	typedef uint8_t MatUpdateFlags_;

	constexpr MatUpdateFlags_ NO_UPDATE_MAT = 0;
	constexpr MatUpdateFlags_ UPDATE_MAT_PRMS = 1;
	constexpr MatUpdateFlags_ UPDATE_MAT_MAPS = 2;
	constexpr MatUpdateFlags_ IS_FIRST_MAT_ALLOC = 4;

	typedef uint8_t MatDescFlags_;

	constexpr MatDescFlags_ SMOOTHNESS_IS_MAP = 1;
	constexpr MatDescFlags_ METALNESS_IS_MAP = 2;

	struct Standard_Material_Desc
	{
		PShader_ID shader_type;
		MatDescFlags_ union_tags;

		PROPERTY()
			dx::XMFLOAT4 base_color;

		PROPERTY()
			wchar_t albedo_path[128];

		union
		{
			float smoothness_value;
			PROPERTY()
				wchar_t smoothness_map[128];
		};

		union
		{
			PROPERTY()
				float metalness_value;
			wchar_t metalness_map[128];
		};

		PROPERTY()
			wchar_t normal_path[128];

		SSS_MODEL selected_sss_model;

		PROPERTY()
			Jimenez_SSS_Model sss_model;


	};


	struct Texture_2D
	{
		ID3D11Texture2D* resource;
	};

	struct Subsurface_Scattering_Params
	{
		float samples[64];
		dx::XMFLOAT4 subsurface_albedo;
		dx::XMFLOAT4 mean_free_path_color;
		float mean_free_path_dist;
		float scale;
		dx::XMFLOAT2 dir;
		uint32_t num_samples;
		uint32_t pad[3];

		dx::XMFLOAT4 jimenez_samples_sss[15];
		//dx::XMFLOAT4 kernel[16];
	};

	struct Material_Params
	{
		dx::XMFLOAT4 base_color;

		uint32_t has_albedo_map;
		uint32_t has_metalness_map;
		uint32_t has_normal_map;
		uint32_t has_smoothness_map;

		float roughness;
		float metalness;

		uint32_t sss_model_id;
		uint8_t pad[4];
	};


	struct Material
	{
		Pixel_Shader model;

		Shader_Texture2D albedo_map;
		Shader_Texture2D metalness_map;
		Shader_Texture2D normal_map;
		Shader_Texture2D smoothness_map;

		Subsurface_Scattering_Params sss_prms;
		Material_Params mat_prms;
	};

}
#endif