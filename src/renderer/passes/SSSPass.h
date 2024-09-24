#ifndef SSSPASS_H_
#define SSSPASS_H_

#include "renderer/backend/RenderGraph.h"
#include "renderer/backend/RenderScope.h"

namespace zorya
{
	struct SSS_Draw_Constants
	{
		float2 dir;
		float sd_gauss;
		float pad;
	};


	class SSS_Pass
	{
	public:
		SSS_Pass(Render_Graph& render_graph, Render_Scope& scope, Arena* arena, Constant_Buffer_Handle m_hnd_frame_cb, Constant_Buffer_Handle m_hnd_object_cb, Constant_Buffer_Handle hnd_sss_draw_constants);
	};
}

#endif // !SSSPASS_H_
