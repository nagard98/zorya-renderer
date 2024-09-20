#ifndef SHADOW_MAPPING_PASS_H_
#define SHADOW_MAPPING_PASS_H_

#include "renderer/backend/RenderGraph.h"
#include "renderer/backend/RenderScope.h"
#include "renderer/frontend/SceneManager.h"

namespace zorya
{
	struct Shadow_Mapping_Data
	{
		Render_Graph_Resource shadow_maps[16];
	};

	class Shadow_Mapping_Pass
	{
	public:
		Shadow_Mapping_Pass(Render_Graph& render_graph, Render_Scope& scope, Arena* arena, const View_Desc& view_desc,/* PSO_Handle hnd_shadow_map_creation_pso,*/
			Constant_Buffer_Handle hnd_world_cb, Constant_Buffer_Handle hnd_shadow_map_cb, Omni_Dir_Shadow_CB& _shadow_map_cb, Render_Rect& shadow_map_viewport, Constant_Buffer_Handle hnd_light_draw_cb);
	};
}

#endif