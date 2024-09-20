#ifndef PRESENT_PASS_H_
#define PRESENT_PASS_H_

#include "renderer/backend/RenderGraph.h"
#include "renderer/backend/RenderScope.h"

namespace zorya
{
	struct Final_Render_Graph_Texture
	{
		Render_RTV_Handle texture;
	};

	class Present_Pass
	{
	public:
		Present_Pass(Render_Graph& render_graph, Render_Scope& scope, Arena* arena);
	};

}

#endif // !PRESENT_PASS_H_
