#ifndef DEBUG_VIEW_PASS_H_
#define DEBUG_VIEW_PASS_H_

#include "renderer/backend/RenderGraph.h"
#include "renderer/backend/RenderScope.h"

namespace zorya
{
	struct Debug_View_Data
	{
		Render_Graph_Resource debug_view;
	};

	class Debug_View_Pass
	{
	public:
		Debug_View_Pass(Render_Graph& render_graph, Render_Scope& scope);
	};
}

#endif // !DEBUG_VIEW_PASS_H_
