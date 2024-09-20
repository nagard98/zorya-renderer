#include "PresentPass.h"

#include "renderer/frontend/PipelineStateManager.h"

#include "renderer/passes/GBufferPass.h"
#include "renderer/passes/SkyboxPass.h"
#include "renderer/passes/LightingPass.h"
#include "renderer/passes/DebugViewPass.h"

namespace zorya
{
	Present_Pass::Present_Pass(Render_Graph& render_graph, Render_Scope& scope, Arena* arena)
	{
		render_graph.add_pass_callback(L"Present Pass", [&](Render_Graph_Builder& builder)
			{
				auto& lighting_data = scope.get<Lighting_Data>();
				auto& debug_data = scope.get<Debug_View_Data>();
				auto& final_data = scope.get<Final_Render_Graph_Texture>();

				//builder.read(gbuff_data.albedo, Bind_Flag::RENDER_TARGET);
				auto output_texture = builder.read(lighting_data.scene_lighted, Bind_Flag::SHADER_RESOURCE);
				//auto output_texture = builder.read(lighting_data.scene_lighted, Bind_Flag::RENDER_TARGET);
				//auto output_texture = builder.read(debug_data.debug_view, Bind_Flag::SHADER_RESOURCE);
				

				return [=](Render_Command_List& cmd_list, Render_Graph_Registry& registry) 
					{
						OutputDebugString("Running Present Pass\n");
						
						Render_RTV_Handle rt_hnds[] = {final_data.texture};

						Constant_Buffer_Handle vs_cb_hnds[] = { 0 };
						Render_SRV_Handle vs_srv_hnds[] = { 0 };
						Constant_Buffer_Handle ps_cb_hnds[] = { 0 };
						Render_SRV_Handle ps_srv_hnds[] = { registry.get<Render_SRV_Handle>(output_texture) };

						//cmd_list.copy_texture(final_data.texture, registry.get<Render_RTV_Handle>(output_texture));
						cmd_list.draw(
							pipeline_state_manager.get(Pipeline_State::COMPOSIT),
							create_shader_render_targets(*arena, rt_hnds, 1, Render_DSV_Handle{ 0 }),
							create_shader_arguments(*arena, vs_cb_hnds, vs_srv_hnds, ps_cb_hnds, ps_srv_hnds),
							nullptr,
							0,
							D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
							Submesh_Handle_t{ 0,0,0,4,0 }
						);

						//Render_RTV_Handle passRenderTargetView_hnds[] = { hnd_final_rtv, hnd_skin_rtv[4] };
						//constant_buffer_handle2 hnd_object_cb{ m_hnd_object_cb.index };

						//constant_buffer_handle2 vs_cb_hnds[] = { 0 };
						//Render_SRV_Handle vs_srv_hnds[] = { 0 };
						//constant_buffer_handle2 ps_cb_hnds[] = { hnd_object_cb };
						//Render_SRV_Handle ps_srv_hnds[] = { hnd_skin_srv[3], hnd_gbuff_srv[G_Buffer::ROUGH_MET], hnd_skin_srv[2], hnd_depth_srv, hnd_gbuff_srv[G_Buffer::ALBEDO], hnd_skin_srv[0] };

						//cmd_list.draw(
						//	hnd_final_composit_pso,
						//	create_shader_render_targets(*arena, passRenderTargetView_hnds, 2, Render_DSV_Handle{ 0 }),
						//	create_shader_arguments(*arena, vs_cb_hnds, vs_srv_hnds, ps_cb_hnds, ps_srv_hnds),
						//	nullptr,
						//	0,
						//	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
						//	Submesh_Handle_t{ 0,0,0,4,0 }
						//);
					};
			}
		);
	}
}