#include "LightingPass.h"
#include "renderer/passes/GBufferPass.h"
#include "renderer/passes/SkyboxPass.h"
#include "renderer/passes/ShadowMappingPass.h"

#include "renderer/frontend/PipelineStateManager.h"

#include "renderer/backend/GraphicsContext.h"
#include "renderer/backend/rhi/RenderHardwareInterface.h"

namespace zorya
{
	Lighting_Pass::Lighting_Pass(Render_Graph& render_graph, Render_Scope& scope, Arena* arena, const View_Desc& view_desc, Constant_Buffer_Handle hnd_inv_mat_cb,
		Constant_Buffer_Handle m_hnd_frame_cb, Constant_Buffer_Handle hnd_omni_dir_shad_cb, Constant_Buffer_Handle hnd_cam_transf_cb, Constant_Buffer_Handle hnd_light_draw_cb)
	{
		render_graph.add_pass_callback(L"Lighting Pass", [&](Render_Graph_Builder& builder) 
			{
				auto& gbuff_data = scope.get<GBuffer_Data>();
				auto albedo = builder.read(gbuff_data.albedo, Bind_Flag::SHADER_RESOURCE);
				auto normal = builder.read(gbuff_data.normal, Bind_Flag::SHADER_RESOURCE);
				auto rough_metalness = builder.read(gbuff_data.roughness_metalness, Bind_Flag::SHADER_RESOURCE);
				auto vert_normal = builder.read(gbuff_data.vertex_normal, Bind_Flag::SHADER_RESOURCE);
				auto stencil = builder.read(gbuff_data.stencil, Bind_Flag::DEPTH_STENCIL);

				auto& shadowmap_data = scope.get<Shadow_Mapping_Data>();
				for (size_t i = 0; i < view_desc.lights_info.size(); i++)
				{
					shadowmap_data.shadow_maps[i] = builder.read(shadowmap_data.shadow_maps[i], Bind_Flag::SHADER_RESOURCE);
				}

				Render_Graph_Resource_Desc shadowmask_desc;
				shadowmask_desc.width = 1920;
				shadowmask_desc.height = 1080;
				shadowmask_desc.format = Format::FORMAT_R8G8B8A8_UNORM;

				auto shadow_mask = builder.create(shadowmask_desc);
				auto shadow_mask_produced = builder.write(shadow_mask, Bind_Flag::RENDER_TARGET);
				auto shadow_mask_consumed = builder.read(shadow_mask_produced, Bind_Flag::SHADER_RESOURCE);

				auto& depth_data = scope.get<Depth_Data>();
				auto depth = builder.read(depth_data.depth, Bind_Flag::SHADER_RESOURCE);

				Render_Graph_Resource_Desc desc;
				desc.width = 1920;
				desc.height = 1080;
				desc.format = Format::FORMAT_R11G11B10_FLOAT;

				Lighting_Data lighting_data;				
				lighting_data.diffuse = builder.create(desc);
				lighting_data.specular = builder.create(desc);
				lighting_data.transmitted = builder.create(desc);
				lighting_data.indirect_light = builder.create(desc);
				
				lighting_data.scene_lighted = builder.write(gbuff_data.indirect_light, Bind_Flag::RENDER_TARGET);
				lighting_data.diffuse = builder.write(lighting_data.diffuse, Bind_Flag::RENDER_TARGET);
				lighting_data.specular = builder.write(lighting_data.specular, Bind_Flag::RENDER_TARGET);
				lighting_data.transmitted = builder.write(lighting_data.transmitted, Bind_Flag::RENDER_TARGET);
				lighting_data.indirect_light = builder.write(lighting_data.indirect_light, Bind_Flag::RENDER_TARGET);

				scope.set(lighting_data);

				return [=](Render_Command_List& cmd_list, Render_Graph_Registry& registry) 
					{
						struct Im { dx::XMMATRIX invProj; dx::XMMATRIX invView; };
						Im im;
						im.invProj = dx::XMMatrixTranspose(dx::XMMatrixInverse(nullptr, view_desc.cam.get_proj_matrix()));
						im.invView = dx::XMMatrixTranspose(dx::XMMatrixInverse(nullptr, view_desc.cam.get_view_matrix()));
						cmd_list.update_buffer(*arena, hnd_inv_mat_cb, &im, sizeof(im));

						auto& lights = view_desc.lights_info;

						auto scene = registry.get<Render_RTV_Handle>(lighting_data.scene_lighted);
						auto diffuse = registry.get<Render_RTV_Handle>(lighting_data.diffuse);
						auto specular = registry.get<Render_RTV_Handle>(lighting_data.specular);
						auto transmitted = registry.get<Render_RTV_Handle>(lighting_data.transmitted);
						auto indirect_light = registry.get<Render_RTV_Handle>(lighting_data.indirect_light);

						dx::XMFLOAT4 clear_col = { 0.0f,0.0f,0.0f,1.0f };
						//cmd_list.clear_rtv(diffuse, &clear_col.x);
						cmd_list.clear_rtv(specular, &clear_col.x);
						cmd_list.clear_rtv(transmitted, &clear_col.x);

						cmd_list.copy_texture(diffuse, scene);

						for(int i=0; i < lights.size(); i++)
						{
							auto& light = lights[i];

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

							cmd_list.begin_event(L"Building Shadow Mask");
							{
								Render_RTV_Handle render_target_hnds_light_pass[] = { registry.get<Render_RTV_Handle>(shadow_mask_produced) };
								auto num_rts = sizeof(render_target_hnds_light_pass) / sizeof(render_target_hnds_light_pass[0]);

								Constant_Buffer_Handle vs_cb_hnds[] = { 0 };
								Render_SRV_Handle vs_srv_hnds[] = { 0 };
								Constant_Buffer_Handle ps_cb_hnds[] = { m_hnd_frame_cb, hnd_inv_mat_cb, hnd_omni_dir_shad_cb, hnd_light_draw_cb };
								Render_SRV_Handle ps_srv_hnds[] = { registry.get<Render_SRV_Handle>(depth), registry.get<Render_SRV_Handle>(normal), registry.get<Render_SRV_Handle>(shadowmap_data.shadow_maps[i]), registry.get<Render_SRV_Handle>(shadowmap_data.shadow_maps[i]) };

								cmd_list.draw(
									pipeline_state_manager.get(Pipeline_State::SHADOW_MASK),
									create_shader_render_targets(*arena, render_target_hnds_light_pass, num_rts, Render_DSV_Handle{ 0 }/*registry.get<Render_DSV_Handle>(stencil)*/),
									create_shader_arguments(*arena, vs_cb_hnds, vs_srv_hnds, ps_cb_hnds, ps_srv_hnds),
									nullptr,
									0,
									D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
									Submesh_Handle_t{ 0,0,0,4,0 }
								);
							}
							cmd_list.end_event();


							Render_RTV_Handle render_target_hnds_light_pass[] = { scene, diffuse, specular, transmitted, indirect_light };
							auto num_rts = sizeof(render_target_hnds_light_pass) / sizeof(render_target_hnds_light_pass[0]);

							Constant_Buffer_Handle vs_cb_hnds[] = { 0 };
							Render_SRV_Handle vs_srv_hnds[] = { 0 };
							Constant_Buffer_Handle ps_cb_hnds[] = { m_hnd_frame_cb, 0 /*hnd_omni_dir_shad_cb*/, 0, hnd_light_draw_cb, 0, hnd_inv_mat_cb };
							Render_SRV_Handle ps_srv_hnds[] = {
								registry.get<Render_SRV_Handle>(albedo),
								registry.get<Render_SRV_Handle>(normal),
								registry.get<Render_SRV_Handle>(rough_metalness),
								registry.get<Render_SRV_Handle>(depth),
								registry.get<Render_SRV_Handle>(shadow_mask_consumed),
								/*hnd_shadow_cubemap_srv*/ 0,
								/*hnd_spot_shadow_map_srv*/ 0,
								registry.get<Render_SRV_Handle>(vert_normal),
							};

							auto as = registry.get<Render_DSV_Handle>(stencil);
							auto ptr = rhi.get_dsv_pointer(as);
							D3D11_DEPTH_STENCIL_VIEW_DESC ds_desc;
							ptr->GetDesc(&ds_desc);

							cmd_list.draw(
								pipeline_state_manager.get(Pipeline_State::LIGHTING),
								create_shader_render_targets(*arena, render_target_hnds_light_pass, num_rts, registry.get<Render_DSV_Handle>(stencil)),
								create_shader_arguments(*arena, vs_cb_hnds, vs_srv_hnds, ps_cb_hnds, ps_srv_hnds),
								nullptr,
								0,
								D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
								Submesh_Handle_t{ 0,0,0,4,0 }
							);
						}
					};
			}
		);
	}
}