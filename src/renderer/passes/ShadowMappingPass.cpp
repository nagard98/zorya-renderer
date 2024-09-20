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

				Shadow_Mapping_Data shadow_map_data;

				for (int i = 0; i < view_desc.lights_info.size(); i++)
				{
					//TODO: fix creation of shadow map; descriptions (at least partial) should be provided by the caller
					shadow_map_data.shadow_maps[i] = builder.create(shadowmap_desc);
					shadow_map_data.shadow_maps[i] = builder.write(shadow_map_data.shadow_maps[i], Bind_Flag::DEPTH_STENCIL);
				}

				scope.set(shadow_map_data);

				auto shadow_map_cb = &_shadow_map_cb;

				return [=](Render_Command_List& cmd_list, Render_Graph_Registry& registry)
					{
						OutputDebugString("Running Shadow Mapping");
						for (size_t i = 0; i < view_desc.lights_info.size(); i++)
						{
							auto& light = view_desc.lights_info[i];

							struct Im2 { uint32_t light_index; uint32_t light_type; };

							Im2 im2{ 0, (uint32_t)light.tag };
							switch (light.tag)
							{
							case zorya::Light_Type::DIRECTIONAL:
								im2.light_index = i;
								break;
							case zorya::Light_Type::POINT:
								im2.light_index = i - view_desc.num_dir_lights;
								break;
							case zorya::Light_Type::SPOT:
								im2.light_index = i - view_desc.num_dir_lights - view_desc.num_point_lights;
								break;
							default:
								break;
							}

							cmd_list.update_buffer(*arena, hnd_light_draw_cb, &im2, sizeof(im2));

							cmd_list.set_viewport(shadow_map_viewport);

							cmd_list.clear_dsv(registry.get<Render_DSV_Handle>(shadow_map_data.shadow_maps[i]));
							//cmd_list.update_buffer(*arena, hnd_view_cb, &tmp_view_cb, sizeof(tmp_view_cb));
							//cmd_list.update_buffer(*arena, hnd_proj_cb, &tmp_proj_cb, sizeof(tmp_proj_cb));
							cmd_list.update_buffer(*arena, hnd_shadow_map_cb, shadow_map_cb, sizeof(*shadow_map_cb));

							for (const Submesh_Render_Data& submesh_info : view_desc.submeshes_info)
							{
								auto world_matrix = dx::XMMatrixTranspose(submesh_info.final_world_transform);
								cmd_list.update_buffer(*arena, hnd_world_cb, &world_matrix, sizeof(world_matrix));

								Constant_Buffer_Handle vs_cb_hnds[] = { hnd_world_cb, /*hnd_view_cb*/0, /*hnd_proj_cb */hnd_shadow_map_cb, hnd_light_draw_cb };
								Render_SRV_Handle vs_srv_hnds[] = { 0 };
								Constant_Buffer_Handle ps_cb_hnds[] = { 0 };
								Render_SRV_Handle ps_srv_hnds[] = { 0 };

								cmd_list.draw_indexed(
									pipeline_state_manager.get(Pipeline_State::SHADOW_MAPPING),
									create_shader_render_targets(*arena, 0, 0, registry.get<Render_DSV_Handle>(shadow_map_data.shadow_maps[i])),
									create_shader_arguments(*arena, vs_cb_hnds, vs_srv_hnds, ps_cb_hnds, ps_srv_hnds),
									submesh_info.submesh_desc.hnd_buffer_gpu_resource,
									&submesh_info.submesh_desc.hnd_buffer_gpu_resource,
									1,
									D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
									submesh_info.submesh_desc.hnd_submesh
								);
							}

							//for (int face = 0; face < 6; face++)
							//{
							//	cmd_list.clear_dsv(hnd_shadow_cubemap_dsv[point_light_idx * 6 + face], true, 1.0f);
							//}

							//for (Submesh_Render_Data const& submesh_info : view_desc.submeshes_info)
							//{

							//	World_CB tmpWCB{ dx::XMMatrixTranspose(submesh_info.final_world_transform) };
							//	tmp_proj_cb.proj_matrix = omni_dir_shad_cb.point_light_proj_mat;

							//	cmd_list.update_buffer(arena, hnd_world_cb, &tmp_world_cb, sizeof(tmp_world_cb));
							//	cmd_list.update_buffer(arena, hnd_proj_cb, &tmp_proj_cb, sizeof(tmp_proj_cb));


							//	for (int face = 0; face < 6; face++)
							//	{
							//		tmp_view_cb.view_matrix = omni_dir_shad_cb.point_light_view_mat[point_light_idx * 6 + face];
							//		cmd_list.update_buffer(arena, hnd_view_cb, &tmp_view_cb, sizeof(tmp_view_cb));

							//		constant_buffer_handle2 vs_cb_hnds[] = { hnd_world_cb, hnd_view_cb, hnd_proj_cb };
							//		Render_SRV_Handle vs_srv_hnds[] = { 0 };
							//		constant_buffer_handle2 ps_cb_hnds[] = { 0 };
							//		Render_SRV_Handle ps_srv_hnds[] = { 0 };

							//		cmd_list.draw_indexed(
							//			hnd_shadow_map_creation_pso,
							//			create_shader_render_targets(arena, 0, 0, hnd_shadow_cubemap_dsv[point_light_idx * 6 + face]),
							//			create_shader_arguments(arena, vs_cb_hnds, vs_srv_hnds, ps_cb_hnds, ps_srv_hnds),
							//			submesh_info.submesh_desc.hnd_buffer_gpu_resource,
							//			&submesh_info.submesh_desc.hnd_buffer_gpu_resource,
							//			1,
							//			D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
							//			submesh_info.submesh_desc.hnd_submesh
							//		);
							//	}
							//}
						}
					};
			}
		);
	}
}