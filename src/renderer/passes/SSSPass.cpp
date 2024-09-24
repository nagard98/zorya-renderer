#include "SSSPass.h"

#include "renderer/frontend/PipelineStateManager.h"

#include "renderer/backend/GraphicsContext.h"
#include "renderer/passes/LightingPass.h"
#include "renderer/passes/GBufferPass.h"

namespace zorya
{
	SSS_Pass::SSS_Pass(Render_Graph& render_graph, Render_Scope& scope, Arena* arena, Constant_Buffer_Handle m_hnd_frame_cb, Constant_Buffer_Handle m_hnd_object_cb, Constant_Buffer_Handle hnd_sss_draw_constants)
	{
		render_graph.add_pass_callback(L"SSS Pass", [&](Render_Graph_Builder& builder)
			{
				auto& lighting_data = scope.get<Lighting_Data>();
				auto diffuse = builder.read(lighting_data.diffuse, Bind_Flag::SHADER_RESOURCE);
				auto specular = builder.read(lighting_data.specular, Bind_Flag::SHADER_RESOURCE);
				auto transmitted = builder.read(lighting_data.transmitted, Bind_Flag::SHADER_RESOURCE);
				auto scene_lighted = lighting_data.scene_lighted = builder.write(lighting_data.scene_lighted, Bind_Flag::RENDER_TARGET);

				auto& gbuff_data = scope.get<GBuffer_Data>();
				auto depth = builder.read(gbuff_data.stencil, Bind_Flag::SHADER_RESOURCE);
				auto stencil = builder.read(gbuff_data.stencil, Bind_Flag::DEPTH_STENCIL);
				auto albedo = builder.read(gbuff_data.albedo, Bind_Flag::SHADER_RESOURCE);

				Render_Graph_Resource_Desc jim_intermediate_tex_desc;
				jim_intermediate_tex_desc.format = Format::FORMAT_R8G8B8A8_UNORM;
				jim_intermediate_tex_desc.width = 1920.0f;
				jim_intermediate_tex_desc.height = 1080.0f;
				jim_intermediate_tex_desc.arr_size = 1;

				auto jim_intermediate_tex = builder.create(jim_intermediate_tex_desc);
				auto jim_intermediate_rtv = builder.write(jim_intermediate_tex, Bind_Flag::RENDER_TARGET);
				auto jim_intermediate_srv = builder.read(jim_intermediate_rtv, Bind_Flag::SHADER_RESOURCE);

				auto jim_gauss_final_composit1 = builder.create(jim_intermediate_tex_desc);
				auto jim_gauss_final_composit2 = builder.create(jim_intermediate_tex_desc);
				auto jim_gauss_biggest_blur1 = builder.create(jim_intermediate_tex_desc);
				auto jim_gauss_biggest_blur2 = builder.create(jim_intermediate_tex_desc);

				auto jim_gauss_final_composit1_rtv = builder.write(jim_gauss_final_composit1, Bind_Flag::RENDER_TARGET);
				auto jim_gauss_final_composit2_rtv = builder.write(jim_gauss_final_composit2, Bind_Flag::RENDER_TARGET);
				auto jim_gauss_biggest_blur1_rtv = builder.write(jim_gauss_biggest_blur1, Bind_Flag::RENDER_TARGET);
				auto jim_gauss_biggest_blur2_rtv = builder.write(jim_gauss_biggest_blur2, Bind_Flag::RENDER_TARGET);

				auto jim_gauss_biggest_blur1_srv = builder.read(jim_gauss_biggest_blur1_rtv, Bind_Flag::SHADER_RESOURCE);
				auto jim_gauss_biggest_blur2_srv = builder.read(jim_gauss_biggest_blur2_rtv, Bind_Flag::SHADER_RESOURCE);
				
				auto jim_gauss_final_composit1_srv = builder.read(jim_gauss_final_composit1_rtv, Bind_Flag::SHADER_RESOURCE);
				auto jim_gauss_final_composit2_srv = builder.read(jim_gauss_final_composit2_rtv, Bind_Flag::SHADER_RESOURCE);

				return [=](Render_Command_List& cmd_list, Render_Graph_Registry& registry)
					{
						cmd_list.begin_event(L"Golubev SSS");
						{
							Render_RTV_Handle render_target_hnds_golubev_sss_pass[] = { registry.get<Render_RTV_Handle>(scene_lighted) , 0};
							u32 num_rts = sizeof(render_target_hnds_golubev_sss_pass) / sizeof(render_target_hnds_golubev_sss_pass[0]);

							Constant_Buffer_Handle vs_cb_hnds[] = { 0 };
							Render_SRV_Handle vs_srv_hnds[] = { 0 };

							Constant_Buffer_Handle ps_cb_hnds[] = { m_hnd_frame_cb, m_hnd_object_cb };
							Render_SRV_Handle ps_srv_hnds[] = { 
								registry.get<Render_SRV_Handle>(diffuse),
								registry.get<Render_SRV_Handle>(specular),
								registry.get<Render_SRV_Handle>(transmitted),
								registry.get<Render_SRV_Handle>(depth),
								registry.get<Render_SRV_Handle>(albedo),
							};

							cmd_list.draw(
								pipeline_state_manager.get(Pipeline_State::SSS_GOLUBEV),
								create_shader_render_targets(*arena, render_target_hnds_golubev_sss_pass, num_rts, registry.get<Render_DSV_Handle>(stencil)),
								create_shader_arguments(*arena, vs_cb_hnds, vs_srv_hnds, ps_cb_hnds, ps_srv_hnds),
								nullptr,
								0,
								D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
								Submesh_Handle_t{ 0,0,0,4,0 }
							);

						}
						cmd_list.end_event();


						cmd_list.begin_event(L"Jimenez Separable SSS");
						{
							cmd_list.begin_event(L"Horizontal Pass");
							{
								Render_RTV_Handle rt_hnds[] = { registry.get<Render_RTV_Handle>(jim_intermediate_rtv), 0};
								u32 num_rts = sizeof(rt_hnds) / sizeof(rt_hnds[0]);

								dx::XMFLOAT4 clear_col = { 0.0f,0.0f,0.0f,1.0f };
								cmd_list.clear_rtv(rt_hnds[0], &clear_col.x);

								float val[2] = { 1.0f, 0.0f };
								cmd_list.update_buffer(*arena, hnd_sss_draw_constants, val, sizeof(val));

								Constant_Buffer_Handle vs_cb_hnds[] = { 0 };
								Render_SRV_Handle vs_srv_hnds[] = { 0 };

								Constant_Buffer_Handle ps_cb_hnds[] = { m_hnd_frame_cb, hnd_sss_draw_constants };
								Render_SRV_Handle ps_srv_hnds[] = {
									registry.get<Render_SRV_Handle>(diffuse),
									registry.get<Render_SRV_Handle>(specular),
									registry.get<Render_SRV_Handle>(transmitted),
									registry.get<Render_SRV_Handle>(depth),
									registry.get<Render_SRV_Handle>(albedo),
								};

								cmd_list.draw(
									pipeline_state_manager.get(Pipeline_State::SSS_JIMENEZ_SEPARABLE),
									create_shader_render_targets(*arena, rt_hnds, num_rts, registry.get<Render_DSV_Handle>(stencil)),
									create_shader_arguments(*arena, vs_cb_hnds, vs_srv_hnds, ps_cb_hnds, ps_srv_hnds),
									nullptr,
									0,
									D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
									Submesh_Handle_t{ 0,0,0,4,0 }
								);
							}
							cmd_list.end_event();

							cmd_list.begin_event(L"Vertical Pass");
							{
								Render_RTV_Handle rt_hnds[] = { registry.get<Render_RTV_Handle>(scene_lighted), 0 };
								u32 num_rts = sizeof(rt_hnds) / sizeof(rt_hnds[0]);

								float val[2] = { 0.0f, 1.0f };
								cmd_list.update_buffer(*arena, hnd_sss_draw_constants, val, sizeof(val));

								Constant_Buffer_Handle vs_cb_hnds[] = { 0 };
								Render_SRV_Handle vs_srv_hnds[] = { 0 };

								Constant_Buffer_Handle ps_cb_hnds[] = { m_hnd_frame_cb, hnd_sss_draw_constants };
								Render_SRV_Handle ps_srv_hnds[] = {
									registry.get<Render_SRV_Handle>(jim_intermediate_srv),
									registry.get<Render_SRV_Handle>(specular),
									registry.get<Render_SRV_Handle>(transmitted),
									registry.get<Render_SRV_Handle>(depth),
									registry.get<Render_SRV_Handle>(albedo),
								};

								cmd_list.draw(
									pipeline_state_manager.get(Pipeline_State::SSS_JIMENEZ_SEPARABLE),
									create_shader_render_targets(*arena, rt_hnds, num_rts, registry.get<Render_DSV_Handle>(stencil)),
									create_shader_arguments(*arena, vs_cb_hnds, vs_srv_hnds, ps_cb_hnds, ps_srv_hnds),
									nullptr,
									0,
									D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
									Submesh_Handle_t{ 0,0,0,4,0 }
								);
							}
							cmd_list.end_event();
						}
						cmd_list.end_event();

						cmd_list.begin_event(L"Jimenez Gauss SSS");
						{

							Render_RTV_Handle rt_hnds[4] = {
								registry.get<Render_RTV_Handle>(jim_gauss_biggest_blur1_rtv),
								0,//registry.get<Render_RTV_Handle>(jim_gauss_biggest_blur1_rtv),//hnd_skin_rtv[1],

								registry.get<Render_RTV_Handle>(scene_lighted),
								registry.get<Render_RTV_Handle>(jim_gauss_biggest_blur2_rtv)
							};


							float blend_factors[4][4] = {
								{1.0f, 1.0f, 1.0f, 1.0f},
								{0.3251f, 0.45f, 0.3583f, 1.0f},
								{0.34f, 0.1864f, 0.0f, 1.0f},
								{0.46f, 0.0f, 0.0402f, 1.0f}
							};

							float gauss_standard_deviations[4] = { 0.08f, 0.2126f, 0.4694f, 1.3169f };
							SSS_Draw_Constants draw_constants;

							for (int i = 0; i < 4; i++){
															
								draw_constants.sd_gauss = gauss_standard_deviations[i];

								cmd_list.begin_event(L"Horizontal Pass");
								{
									draw_constants.dir = { 1.0f, 0.0f };
									cmd_list.update_buffer(*arena, hnd_sss_draw_constants, &draw_constants, sizeof(draw_constants));

									Constant_Buffer_Handle vs_cb_hnds[] = { 0 };
									Render_SRV_Handle vs_srv_hnds[] = { 0 };

									Constant_Buffer_Handle ps_cb_hnds[] = { m_hnd_frame_cb, hnd_sss_draw_constants };
									Render_SRV_Handle ps_srv_hnds[] = {
										i == 0 ? registry.get<Render_SRV_Handle>(diffuse) : registry.get<Render_SRV_Handle>(jim_gauss_biggest_blur2_srv),
										registry.get<Render_SRV_Handle>(specular),
										registry.get<Render_SRV_Handle>(transmitted),
										registry.get<Render_SRV_Handle>(depth),
										registry.get<Render_SRV_Handle>(albedo),
									};

									cmd_list.draw(
										pipeline_state_manager.get(Pipeline_State::SSS_JIMENEZ_GAUSS_HOR),
										create_shader_render_targets(*arena, rt_hnds, 2, registry.get<Render_DSV_Handle>(stencil)),
										create_shader_arguments(*arena, vs_cb_hnds, vs_srv_hnds, ps_cb_hnds, ps_srv_hnds),
										nullptr,
										0,
										D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
										Submesh_Handle_t{ 0,0,0,4,0 }
									);
								}
								cmd_list.end_event();


								cmd_list.begin_event(L"Vertical Pass");
								{
									draw_constants.dir = { 0.0f, 1.0f };
									cmd_list.update_buffer(*arena, hnd_sss_draw_constants, &draw_constants, sizeof(draw_constants));

									Constant_Buffer_Handle vs_cb_hnds[] = { 0 };
									Render_SRV_Handle vs_srv_hnds[] = { 0 };

									Constant_Buffer_Handle ps_cb_hnds[] = { m_hnd_frame_cb, hnd_sss_draw_constants };
									Render_SRV_Handle ps_srv_hnds[] = {
										registry.get<Render_SRV_Handle>(jim_gauss_biggest_blur1_srv),
										registry.get<Render_SRV_Handle>(specular),
										registry.get<Render_SRV_Handle>(transmitted),
										registry.get<Render_SRV_Handle>(depth),
										registry.get<Render_SRV_Handle>(albedo),
									};

									cmd_list.draw(
										pipeline_state_manager.get(Pipeline_State::SSS_JIMENEZ_GAUSS_VERT),
										create_shader_render_targets(*arena, &rt_hnds[2], 2, registry.get<Render_DSV_Handle>(stencil)),
										create_shader_arguments(*arena, vs_cb_hnds, vs_srv_hnds, ps_cb_hnds, ps_srv_hnds),
										nullptr,
										0,
										D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
										Submesh_Handle_t{ 0,0,0,4,0 },
										blend_factors[i]
									);
								}
								cmd_list.end_event();

							}

						}
						cmd_list.end_event();

					};
			}
		);
	}
}
