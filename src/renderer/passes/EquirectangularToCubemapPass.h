#ifndef EQUIRECTANGULAR_TO_CUBEMAP_PASS_H_
#define EQUIRECTANGULAR_TO_CUBEMAP_PASS_H_

#include "renderer/backend/RenderGraph.h"
#include "renderer/backend/RenderScope.h"
#include "renderer/frontend/SceneManager.h"

namespace zorya
{
	struct Environment_Map_Data
	{
		Render_Graph_Resource environment_map;
	};

	struct Irradiance_Data
	{
		Render_Graph_Resource irradiance_map;
		Render_Graph_Resource pre_filtered_env_map;
		Render_Graph_Resource brdf_lut_map;;
	};

	class Equirectangular_To_Cubemap_Pass
	{
	public:
		Equirectangular_To_Cubemap_Pass(Render_Graph& render_graph, Render_Scope& scope, Arena* arena, Constant_Buffer_Handle hnd_world_cb, Constant_Buffer_Handle hnd_view_cb
			, Constant_Buffer_Handle hnd_proj_cb, Constant_Buffer_Handle hnd_ibl_spec_draw_cb, Render_SRV_Handle hnd_environment_map_srv
			, Render_RTV_Handle hnd_skybox_map_rtv , Render_RTV_Handle hnd_irradiance_map_rtv, Render_RTV_Handle hnd_prefiltered_env_map_rtv, Render_RTV_Handle hnd_brdf_lut_map_rtv);
	};

}

#endif // !EQUIRECTANGULAR_TO_CUBEMAP_PASS_H_