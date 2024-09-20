#ifndef SKYBOX_PASS_H_
#define SKYBOX_PASS_H_

#include "renderer/backend/RenderGraph.h"
#include "renderer/backend/RenderScope.h"
#include "renderer/frontend/SceneManager.h"
#include "renderer/backend/rhi/RenderDeviceHandles.h"

#include "utils/Arena.h"

namespace zorya
{
	struct Skybox_Data
	{
		Render_Graph_Resource skybox;
	};

	struct Depth_Data
	{
		Render_Graph_Resource depth;
	};


	class Skybox_Pass
	{
	public:
		Skybox_Pass(Render_Graph& render_graph, Render_Scope& scope, Render_SRV_Handle sky_cubemap, const View_Desc& view_desc, Arena* arena,
			Constant_Buffer_Handle hnd_world_cb, Constant_Buffer_Handle hnd_view_cb, Constant_Buffer_Handle hnd_proj_cb, Render_Rect& scene_viewport);
	};

}

#endif // !SKYBOX_PASS_H_
