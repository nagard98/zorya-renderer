#ifndef MATERIAL_H_
#define MATERIAL_H_

#include <DirectXMath.h>
#include <string>
#include <cstdint>
#include <d3d11.h>
#include <vector>

#include "Shader.h"
#include "reflection/ReflectionAnnotation.h"
#include "Asset.h"
#include "Texture2D.h"
#include "renderer/frontend/FrontendHandles.h"
#include "renderer/backend/rhi/RenderDeviceHandles.h"

namespace zorya
{
	namespace dx = DirectX;


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

		uint32_t num_supersamples;
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

		//SSS_MODEL selected_sss_model;

		PROPERTY()
			Jimenez_SSS_Model sss_model;


	};


	struct Texture_2D
	{
		ID3D11Texture2D* resource;
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

		dx::XMFLOAT2 dir;
		uint32_t sss_model_id;
		uint32_t sss_model_arr_index;

		float sd_gauss_jim;
		float pad;
	};


	//struct Material3
	//{
	//	Pixel_Shader model;

	//	Shader_Texture2D* albedo_map;
	//	Shader_Texture2D* metalness_map;
	//	Shader_Texture2D* normal_map;
	//	Shader_Texture2D* smoothness_map;

	//	Subsurface_Scattering_Params sss_prms;
	//	Material_Params mat_prms;
	//};

	enum Shading_Model : u32
	{
		DEFAULT_LIT,
		SUBSURFACE_GOLUBEV,
		NUM_SHADING_MODELS
	};

	static const char* shading_model_names = {"Default Lit\0Subsurface Golubev\0"};

	struct Material : public Asset
	{
		Material() : Asset() {};
		
		static Material* create(const Asset_Import_Config* import_config);

		Pixel_Shader shader;
		std::vector<Shader_Resource> resources;
		uint32_t flags;

		Material_Cache_Handle_t material_hnd; //shouldn't be here
		//PSO_Handle pso_hnd;
		Shading_Model shading_model;

		bool is_sss_enabled{ false };
		Diffusion_Profile_Handle diff_prof_hnd{ 0 };

		void load_asset(const Asset_Import_Config* asset_imp_conf) override;

		int serialize();
		int deserialize();

		std::vector<Shader_Resource*> get_material_editor_resources();
	};

	void update_standard_internals(Material& material);

}
#endif