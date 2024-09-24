#ifndef DIFFUSION_PROFILE_H_
#define DIFFUSION_PROFILE_H_

#include "Asset.h"
#include "Platform.h"
#include "renderer/frontend/FrontendHandles.h"

namespace zorya
{
	struct Subsurface_Scattering_Params
	{
		float samples[64];
		float4 subsurface_albedo{0.436, 0.227, 0.131, 1.0f};
		float4 mean_free_path_color{ 3.67, 1.37, 0.68, 1.0f };
		float mean_free_path_dist{ 1.0f };
		float scale{ 1.0f };
		float2 dir;
		u32 num_samples = 21;
		u32 num_supersamples;
		u32 pad[2];

		float4 jimenez_samples_sss[15];
		//dx::XMFLOAT4 kernel[16];
	};

	enum class SSS_MODEL : uint32_t
	{
		NONE,
		JIMENEZ_GAUSS,
		JIMENEZ_SEPARABLE,
		GOLUBEV
	};

	static const char* sss_model_names[] = { "None\0Jimenez Gauss\0Jimenez Separable\0Golubev\0" };

	struct Diffusion_Profile : public Asset
	{
		Diffusion_Profile() : Asset() {}
		Diffusion_Profile(const Asset_Import_Config* ass_imp_config) : Asset(ass_imp_config) {}

		static Diffusion_Profile* create(const Asset_Import_Config* ass_imp_config);

		void load_asset(const Asset_Import_Config* asset_imp_conf) override;

		int serialize();
		int deserialize();

		Diffusion_Profile_Handle hnd;
		SSS_MODEL model_id{SSS_MODEL::GOLUBEV};
		Subsurface_Scattering_Params sss_params{};

	};

}

#endif // !DIFFUSION_PROFILE_H_
