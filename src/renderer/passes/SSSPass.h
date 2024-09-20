#ifndef SSSPASS_H_
#define SSSPASS_H_

#include "renderer/backend/RenderGraph.h"
#include "renderer/backend/RenderScope.h"

namespace zorya
{
	class SSS_Pass
	{
	public:
		SSS_Pass(Render_Graph& render_graph, Render_Scope& scope, Arena* arena, Constant_Buffer_Handle m_hnd_frame_cb, Constant_Buffer_Handle m_hnd_object_cb);
	};
}

#endif // !SSSPASS_H_
