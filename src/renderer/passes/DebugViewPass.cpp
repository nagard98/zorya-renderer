#include "DebugViewPass.h"

#include "renderer/passes/SkyboxPass.h"
#include "renderer/passes/LightingPass.h"
#include "renderer/passes/GBufferPass.h"

namespace zorya
{
	Debug_View_Pass::Debug_View_Pass(Render_Graph& render_graph, Render_Scope& scope)
	{
		render_graph.add_pass_callback(L"Debug View Pass", [&](Render_Graph_Builder& builder)
			{
				Debug_View_Data debug_data;

				//auto& skybox_data = scope.get<Skybox_Data>();	
				auto& gbuff_data = scope.get<GBuffer_Data>();	

				
				Render_Graph_Resource_Desc desc;
				desc.width = 1920;
				desc.height = 1080;
				desc.format = Format::FORMAT_R8G8B8A8_UNORM;

				//debug_data.debug_view = builder.create(desc);
				debug_data.debug_view = builder.write(gbuff_data.normal, Bind_Flag::RENDER_TARGET);

				scope.set(debug_data);

				return [=](Render_Command_List& cmd_list, const Render_Graph_Registry& registry)
					{
						OutputDebugString("Running Debug View Pass\n");
					};
			}
		);
	}
}