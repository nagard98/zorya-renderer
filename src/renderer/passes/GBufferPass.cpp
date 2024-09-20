#include "GBufferPass.h"
#include "renderer/backend/GraphicsContext.h"
#include "renderer/passes/SkyboxPass.h"
#include "renderer/passes/PresentPass.h"
#include "renderer/frontend/PipelineStateManager.h"

namespace zorya
{
	GBuffer_Pass::GBuffer_Pass(Render_Graph& render_graph, Render_Scope& scope, Arena* arena, const View_Desc& view_desc,
		Constant_Buffer_Handle hnd_world_cb, Constant_Buffer_Handle hnd_view_cb, Constant_Buffer_Handle hnd_proj_cb)
	{
		struct World_CB
		{
			dx::XMMATRIX world_matrix;
		};

		render_graph.add_pass_callback(L"Early Declaration Pass", [&](Render_Graph_Builder& builder)
			{
				Render_Graph_Resource_Desc depth_tex_desc;
				depth_tex_desc.width = 1920;
				depth_tex_desc.height = 1080;
				depth_tex_desc.format = Format::FORMAT_D24_UNORM_S8_UINT;

				Depth_Data depth_data;
				depth_data.depth = builder.create(depth_tex_desc);
				
				scope.set(depth_data);

				return [=](Render_Command_List& cmd_list, Render_Graph_Registry& registry)
					{

					};
			}
		);

		render_graph.add_pass_callback(L"G-Buffer Pass", [&](Render_Graph_Builder& builder)
			{
				auto& skybox_data = scope.get<Skybox_Data>();

				Render_Graph_Resource_Desc gbuff_tex_desc;
				gbuff_tex_desc.width = 1920;
				gbuff_tex_desc.height = 1080;
				gbuff_tex_desc.format = Format::FORMAT_R8G8B8A8_UNORM;

				Render_Graph_Resource_Desc stencil_desc;
				stencil_desc.width = 1920;
				stencil_desc.height = 1080;
				stencil_desc.format = Format::FORMAT_D24_UNORM_S8_UINT;

				GBuffer_Data gbuff_data;
				gbuff_data.albedo = builder.create(gbuff_tex_desc);
				gbuff_data.normal = builder.create(gbuff_tex_desc);
				gbuff_data.roughness_metalness = builder.create(gbuff_tex_desc);
				gbuff_data.vertex_normal = builder.create(gbuff_tex_desc);

				gbuff_data.stencil = builder.create(stencil_desc);

				gbuff_data.indirect_light = builder.write(skybox_data.skybox, Bind_Flag::RENDER_TARGET);
				gbuff_data.albedo = builder.write(gbuff_data.albedo, Bind_Flag::RENDER_TARGET);
				gbuff_data.normal = builder.write(gbuff_data.normal, Bind_Flag::RENDER_TARGET);
				gbuff_data.roughness_metalness = builder.write(gbuff_data.roughness_metalness, Bind_Flag::RENDER_TARGET);
				gbuff_data.vertex_normal = builder.write(gbuff_data.vertex_normal, Bind_Flag::RENDER_TARGET);

				auto& depth_data = scope.get<Depth_Data>();
				auto depth_sten_tex = builder.read(depth_data.depth, Bind_Flag::DEPTH_STENCIL);
				depth_data.depth = gbuff_data.stencil = builder.write(depth_sten_tex, Bind_Flag::DEPTH_STENCIL);

				scope.set(gbuff_data);


				return [=](Render_Command_List& cmd_list, Render_Graph_Registry& registry)
					{
						OutputDebugString("Running GBuffer Pass\n");

						auto tmpVCB{ view_desc.cam.get_view_matrix_transposed() };
						auto tmpPCB{ view_desc.cam.get_proj_matrix_transposed() };

						cmd_list.update_buffer(*arena, hnd_view_cb, &tmpVCB, sizeof(tmpVCB));
						cmd_list.update_buffer(*arena, hnd_proj_cb, &tmpPCB, sizeof(tmpPCB));

						///
						Constant_Buffer_Data* cb_data = nullptr;
						Shader_Resource* cb_resource = nullptr;
						///

						dx::XMFLOAT4 clear_stencil_color = { 0.0f,0.0f,0.0f,0.0f };
						/*cmd_list.clear_rtv(hnd_gbuff_rtv[G_Buffer::NORMAL], &clear_color.x);
						cmd_list.clear_rtv(hnd_gbuff_rtv[G_Buffer::ROUGH_MET], &clear_color.x);*/
						//cmd_list.clear_rtv(registry.get<Render_RTV_Handle>(gbuff_data.stencil), &clear_stencil_color.x);
						cmd_list.clear_dsv(registry.get<Render_DSV_Handle>(depth_sten_tex), true, 1.0f);
							
						Render_RTV_Handle rt2_hnds[] = {  
							registry.get<Render_RTV_Handle>(gbuff_data.indirect_light),
							registry.get<Render_RTV_Handle>(gbuff_data.albedo),
							registry.get<Render_RTV_Handle>(gbuff_data.normal), 
							registry.get<Render_RTV_Handle>(gbuff_data.roughness_metalness), 
							registry.get<Render_RTV_Handle>(gbuff_data.vertex_normal),
						};

						Constant_Buffer_Handle vs_cb_hnds[] = { hnd_world_cb, hnd_view_cb, hnd_proj_cb };

						int sss_submesh_count = 0;
						for (Submesh_Render_Data const& submesh_info : view_desc.submeshes_info)
						{
							World_CB tmpOCB{ dx::XMMatrixTranspose(submesh_info.final_world_transform) };

							Material& mat = resource_cache.m_material_cache.at(submesh_info.submesh_desc.hnd_material_cache.index);
								
							Render_SRV_Handle nil_srv{ 0 };

							Constant_Buffer_Handle ps_cb_hnds[128] = { 0 };
							Render_SRV_Handle ps_srv_hnds[128] = { 0 };
							uint32_t num_ps_srv_hnds = 0, num_ps_cb_hnds = 0;

							for (auto resource : mat.get_material_editor_resources())
							{
								switch (resource->m_type)
								{
								case (Resource_Type::ZRY_TEX): {
									//TODO: doesnt consider case where bind_count > 1
									ps_srv_hnds[resource->m_bind_point] = resource->m_hnd_gpu_srv;
									num_ps_srv_hnds = resource->m_bind_point + 1;
									break;
								}
									
								case (Resource_Type::ZRY_CBUFF): {
									cb_resource = resource;
									cb_data = cb_resource->m_parameters;
									//TODO: temporary hardcoded value for size
									cmd_list.update_buffer(*arena, resource->m_hnd_gpu_cb, cb_data->cb_start, 52);
									ps_cb_hnds[resource->m_bind_point] = resource->m_hnd_gpu_cb;
									num_ps_cb_hnds = resource->m_bind_point + 1;
						
									//TODO: get mat_prms buff directly instead checking each resource
									//TODO: fix - don't use submesh count for sss model arr index (need to implement asset register)
									//uint32_t sss_model_id;
									//if (get_constant_buff_var(cb_data, "_sssModelId", &sss_model_id))
									//{
									//	if (sss_model_id != 0)
									//	{
									//		//sss_type_in_scene[sss_model_id] = true;
									//		bool suc = set_constant_buff_var(cb_data, "sssModelArrIndex", &sss_submesh_count, sizeof(sss_submesh_count));
									//		assert(suc);
									//		memcpy_s(((tmp_sss_params_cb + sss_model_id) + (sss_submesh_count << 2))->jimenez_samples_sss, sizeof(dx::XMFLOAT4) * num_samples, kernel.data(), sizeof(dx::XMFLOAT4) * num_samples);
									//		sss_submesh_count += 1;
									//	}
									//} else
									//{
									//	uint32_t val = 0;
									//	bool suc = set_constant_buff_var(cb_data, "sssModelArrIndex", &val, sizeof(val));
									//	assert(suc);
									//	//mat.mat_prms.sss_model_arr_index = 0;
									//}
									//	
									//break;
								}
									
								default:
									break;
								}
							}

							//-------------------------------------------------
								
							cmd_list.update_buffer(*arena, hnd_world_cb, &tmpOCB, sizeof(tmpOCB));
								
							Render_SRV_Handle vs_srv_hnds[] = { 0 };
								
							cmd_list.draw_indexed(
								pipeline_state_manager.get(mat.shading_model),
								create_shader_render_targets(*arena, rt2_hnds, sizeof(rt2_hnds)/sizeof(rt2_hnds[0]), registry.get<Render_DSV_Handle>(depth_sten_tex)),
								create_shader_arguments(*arena, vs_cb_hnds, sizeof(vs_cb_hnds)/sizeof(vs_cb_hnds[0]), vs_srv_hnds, 0, ps_cb_hnds, num_ps_cb_hnds, ps_srv_hnds, num_ps_srv_hnds),
								submesh_info.submesh_desc.hnd_buffer_gpu_resource,
								&submesh_info.submesh_desc.hnd_buffer_gpu_resource,
								1,
								D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
								submesh_info.submesh_desc.hnd_submesh
								);
						}

					};
			}
		);
	}
}