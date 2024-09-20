#ifndef GBUFFER_PASS_H_
#define GBUFFER_PASS_H_

#include "renderer/frontend/SceneManager.h"
#include "renderer/backend/RenderScope.h"
#include "renderer/backend/RenderGraph.h"

namespace zorya
{
	struct GBuffer_Data
	{
		Render_Graph_Resource indirect_light;
		Render_Graph_Resource albedo;
		Render_Graph_Resource normal;
		Render_Graph_Resource roughness_metalness;
		Render_Graph_Resource vertex_normal;
		Render_Graph_Resource stencil;
	};

	class GBuffer_Pass
	{
	public:
		GBuffer_Pass(Render_Graph& render_graph, Render_Scope& scope, Arena* arena, const View_Desc& view_desc,
			Constant_Buffer_Handle hnd_world_cb, Constant_Buffer_Handle hnd_view_cb, Constant_Buffer_Handle hnd_proj_cb);
	};
}

#endif