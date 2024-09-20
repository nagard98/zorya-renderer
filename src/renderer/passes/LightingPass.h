#ifndef LIGHTING_PASS_H_
#define LIGHTING_PASS_H_

#include "renderer/backend/RenderGraph.h"
#include "renderer/backend/RenderScope.h"

#include "renderer/frontend/SceneManager.h"

namespace zorya
{
	struct Lighting_Data
	{
		Render_Graph_Resource scene_lighted;
		Render_Graph_Resource diffuse;
		Render_Graph_Resource specular;
		Render_Graph_Resource transmitted;
		Render_Graph_Resource indirect_light;
	};

	class Lighting_Pass
	{
	public:
		Lighting_Pass(Render_Graph& render_graph, Render_Scope& scope, Arena* arena, const View_Desc& view_desc, Constant_Buffer_Handle hnd_inv_mat_cb,
			Constant_Buffer_Handle m_hnd_frame_cb, Constant_Buffer_Handle hnd_omni_dir_shad_cb, Constant_Buffer_Handle hnd_cam_cb, Constant_Buffer_Handle hnd_light_draw_cb);
	};
}

#endif