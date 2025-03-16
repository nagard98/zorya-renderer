#include "ShadowMappingPass.h"

#include "renderer/backend/GraphicsContext.h"
#include "renderer/frontend/PipelineStateManager.h"

namespace zorya
{
	Shadow_Mapping_Pass::Shadow_Mapping_Pass(Render_Graph& render_graph, Render_Scope& scope, Arena* arena, const View_Desc& view_desc,/* PSO_Handle hnd_shadow_map_creation_pso,*/
		Constant_Buffer_Handle hnd_world_cb, Constant_Buffer_Handle hnd_shadow_map_cb, Omni_Dir_Shadow_CB& _shadow_map_cb, Render_Rect& shadow_map_viewport, Constant_Buffer_Handle hnd_light_draw_cb)
	{
		render_graph.add_pass_callback(L"Shadow Mapping", [&](Render_Graph_Builder& builder)
			{
				Render_Graph_Resource_Desc shadowmap_desc;
				shadowmap_desc.width = 4096;
				shadowmap_desc.height = 4096;
				shadowmap_desc.format = Format::FORMAT_D32_FLOAT;

				Render_Graph_Resource_Desc shadowmap_point_desc;
				shadowmap_point_desc.width = 4096;
				shadowmap_point_desc.height = 4096;
				shadowmap_point_desc.format = Format::FORMAT_D32_FLOAT;
				shadowmap_point_desc.arr_size = 6;

				Shadow_Mapping_Data shadow_map_data;

				//TODO: fix all indexing of lights in pass
				Render_Graph_Resource dsv_shadowmaps[16];
				Render_Graph_Resource dsv_cube_shadowmaps[6*16];

				for (int i = 0, i_point = 0; i < view_desc.lights_info.size(); i++)
				{
					auto& light = view_desc.lights_info[i];
					//TODO: fix creation of shadow map; descriptions (at least partial) should be provided by the caller
					switch (light.tag)
					{
					case zorya::Light_Type::SKYLIGHT:
					case zorya::Light_Type::DIRECTIONAL:
					case zorya::Light_Type::SPOT:
					{
						shadow_map_data.shadow_maps[i] = builder.create(shadowmap_desc);
						shadow_map_data.shadow_maps[i] = dsv_shadowmaps[i] = builder.write(shadow_map_data.shadow_maps[i], Bind_Flag::DEPTH_STENCIL);
						break;
					}
					case zorya::Light_Type::POINT:
					{
						shadow_map_data.shadow_maps[i] = builder.create(shadowmap_point_desc);

						for (int face = 0; face < 6; face++)
						{
							shadow_map_data.shadow_maps[i] = dsv_cube_shadowmaps[i_point * 6 + face] = builder.write(shadow_map_data.shadow_maps[i], Bind_Flag::DEPTH_STENCIL, face, 1);
						}
						i_point += 1;
						break;
					}

					default:
						zassert(false);
					}

				}

				scope.set(shadow_map_data);

				auto shadow_map_cb = &_shadow_map_cb;

				return [=](Render_Command_List& cmd_list, Render_Graph_Registry& registry)
					{
						cmd_list.update_buffer(*arena, hnd_shadow_map_cb, shadow_map_cb, sizeof(*shadow_map_cb));

						for (size_t i = 0; i < view_desc.lights_info.size(); i++)
						{
							auto& light = view_desc.lights_info[i];

							struct Draw_Constants { uint32_t light_index; uint32_t light_type; uint32_t face;};

							Draw_Constants draw_constants{ 0, (uint32_t)light.tag, 0};
							switch (light.tag)
							{
							case zorya::Light_Type::DIRECTIONAL:
								draw_constants.light_index = i;
								break;
							case zorya::Light_Type::POINT:
								draw_constants.light_index = i - view_desc.num_dir_lights;
								break;
							case zorya::Light_Type::SPOT:
								draw_constants.light_index = i - view_desc.num_dir_lights - view_desc.num_point_lights;
								break;
							default:
								break;
							}

							cmd_list.set_viewport(shadow_map_viewport);

							switch (light.tag)
							{
							case zorya::Light_Type::DIRECTIONAL:
							case zorya::Light_Type::SPOT:
							{
								cmd_list.update_buffer(*arena, hnd_light_draw_cb, &draw_constants, sizeof(draw_constants));

								cmd_list.clear_dsv(registry.get<Render_DSV_Handle>(dsv_shadowmaps[i]), true, 0.0f);

								for (const Submesh_Render_Data& submesh_info : view_desc.submeshes_info)
								{
									auto world_matrix = dx::XMMatrixTranspose(submesh_info.final_world_transform);
									cmd_list.update_buffer(*arena, hnd_world_cb, &world_matrix, sizeof(world_matrix));

									Constant_Buffer_Handle vs_cb_hnds[] = { hnd_world_cb, Constant_Buffer_Handle{0}, hnd_shadow_map_cb, hnd_light_draw_cb };
									Render_SRV_Handle vs_srv_hnds[] = { Render_SRV_Handle{0} };
									Constant_Buffer_Handle ps_cb_hnds[] = { Constant_Buffer_Handle{0} };
									Render_SRV_Handle ps_srv_hnds[] = { Render_SRV_Handle{0} };

									cmd_list.draw_indexed(
										pipeline_state_manager.get(Pipeline_State::SHADOW_MAPPING),
										create_shader_render_targets(*arena, 0, 0, registry.get<Render_DSV_Handle>(dsv_shadowmaps[i])),
										create_shader_arguments(*arena, vs_cb_hnds, vs_srv_hnds, ps_cb_hnds, ps_srv_hnds),
										submesh_info.submesh_desc.hnd_buffer_gpu_resource,
										&submesh_info.submesh_desc.hnd_buffer_gpu_resource,
										1,
										D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
										submesh_info.submesh_desc.hnd_submesh
									);
								}
								break;
							}
							case zorya::Light_Type::POINT:
							{
								for (int face = 0; face < 6; face++)
								{
									cmd_list.clear_dsv(registry.get<Render_DSV_Handle>(dsv_cube_shadowmaps[draw_constants.light_index * 6 + face]), true, 0.0f);
								}

								for (Submesh_Render_Data const& submesh_info : view_desc.submeshes_info)
								{
									auto tmpWCB = dx::XMMatrixTranspose(submesh_info.final_world_transform);
									cmd_list.update_buffer(*arena, hnd_world_cb, &tmpWCB, sizeof(tmpWCB));

									for (int face = 0; face < 6; face++)
									{
										draw_constants.face = face;
										cmd_list.update_buffer(*arena, hnd_light_draw_cb, &draw_constants, sizeof(draw_constants));

										Constant_Buffer_Handle vs_cb_hnds[] = { hnd_world_cb, Constant_Buffer_Handle{0}, hnd_shadow_map_cb, hnd_light_draw_cb };
										Render_SRV_Handle vs_srv_hnds[] = { Render_SRV_Handle{0} };
										Constant_Buffer_Handle ps_cb_hnds[] = { Constant_Buffer_Handle{0} };
										Render_SRV_Handle ps_srv_hnds[] = { Render_SRV_Handle{0} };

										cmd_list.draw_indexed(
											pipeline_state_manager.get(Pipeline_State::SHADOW_MAPPING),
											create_shader_render_targets(*arena, 0, 0, registry.get<Render_DSV_Handle>(dsv_cube_shadowmaps[draw_constants.light_index * 6 + face])),
											create_shader_arguments(*arena, vs_cb_hnds, vs_srv_hnds, ps_cb_hnds, ps_srv_hnds),
											submesh_info.submesh_desc.hnd_buffer_gpu_resource,
											&submesh_info.submesh_desc.hnd_buffer_gpu_resource,
											1,
											D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
											submesh_info.submesh_desc.hnd_submesh
										);
									}
								}
								break;
							}
							default:
								break;
							}

						}
					};
			}
		);
	}
}