#include "EquirectangularToCubemapPass.h"
#include "renderer/backend/GraphicsContext.h"
#include "renderer/frontend/PipelineStateManager.h"
#include "renderer/passes/SkyboxPass.h"

namespace zorya
{
	void convolve_cubemap_face(Arena* arena, Render_Command_List& cmd_list, Render_Graph_Registry& registry, const Render_Graph_Resource& src_map, const Render_Graph_Resource& dest_face_rtv,
		Constant_Buffer_Handle hnd_world_cb, Constant_Buffer_Handle hnd_view_cb, Constant_Buffer_Handle hnd_ibl_spec_draw_cb, Constant_Buffer_Handle hnd_proj_cb);

	Equirectangular_To_Cubemap_Pass::Equirectangular_To_Cubemap_Pass(Render_Graph& render_graph, Render_Scope& scope, Arena* arena, Constant_Buffer_Handle hnd_world_cb
		, Constant_Buffer_Handle hnd_view_cb, Constant_Buffer_Handle hnd_proj_cb, Constant_Buffer_Handle hnd_ibl_spec_draw_cb, Render_SRV_Handle hnd_environment_map_srv
		, Render_RTV_Handle hnd_skybox_map_rtv, Render_RTV_Handle hnd_irradiance_map_rtv, Render_RTV_Handle hnd_prefiltered_env_map_rtv, Render_RTV_Handle hnd_brdf_lut_map_rtv)
	{
		render_graph.add_pass_callback(L"Equirectangular Conversion", [&](Render_Graph_Builder& builder)
			{
				Render_Rect viewport;
				viewport.top_left_x = 0;
				viewport.top_left_y = 0;
				viewport.width = 512;
				viewport.height = 512;

				Render_Graph_Resource_Desc environment_map_desc;
				environment_map_desc.arr_size = 6;
				environment_map_desc.format = Format::FORMAT_R11G11B10_FLOAT;
				environment_map_desc.width = viewport.width;
				environment_map_desc.height = viewport.height;
				environment_map_desc.is_cubemap = true;

				Environment_Map_Data environment_map_data;
				auto environment_map = environment_map_data.environment_map = builder.create(environment_map_desc);

				Render_Graph_Resource environment_map_rts[6];
				for (size_t i = 0; i < 6; i++)
				{
					environment_map_rts[i] = builder.write(environment_map, Bind_Flag::RENDER_TARGET, i, 1);
					builder.read(environment_map_rts[i], Bind_Flag::SHADER_RESOURCE, i, 1);
				}

				scope.set(environment_map_data);


				return [=](Render_Command_List& cmd_list, Render_Graph_Registry& registry)
					{
						auto matrix_identity = DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity());
						auto proj_mat = dx::XMMatrixTranspose(dx::XMMatrixPerspectiveFovLH(dx::XM_PIDIV2, 1.0f, 100.0f, 0.1f));
						cmd_list.update_buffer(*arena, hnd_world_cb, &matrix_identity, sizeof(matrix_identity));
						cmd_list.update_buffer(*arena, hnd_proj_cb, &proj_mat, sizeof(proj_mat));

						//TODO: create default set of these view matrices
						DirectX::XMMATRIX view_matrices[6];
						view_matrices[0] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(dx::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
						view_matrices[1] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(dx::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
						view_matrices[2] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(dx::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f)));
						view_matrices[3] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(dx::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f)));
						view_matrices[4] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(dx::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
						view_matrices[5] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(dx::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));

						cmd_list.set_viewport(viewport);

						for (size_t i = 0; i < 6; i++)
						{
							cmd_list.update_buffer(*arena, hnd_view_cb, &view_matrices[i], sizeof(view_matrices[i]));

							Render_RTV_Handle rts[] = { registry.get<Render_RTV_Handle>(environment_map_rts[i])};

							Constant_Buffer_Handle vs_cb_hnds[] = { hnd_world_cb, hnd_view_cb, hnd_proj_cb };
							Render_SRV_Handle vs_srv_hnds[] = { 0 };
							Constant_Buffer_Handle ps_cb_hnds[] = { 0 };
							Render_SRV_Handle ps_srv_hnds[] = { hnd_environment_map_srv };
							Buffer_Handle_t* nil_buff_hnd = {};
							Submesh_Handle_t hnd_submesh;
							hnd_submesh.num_vertices = 36;


							cmd_list.draw(
								pipeline_state_manager.get(Pipeline_State::EQUIRECTANGULAR_CONVERSION),
								create_shader_render_targets(*arena, rts, 1, Render_DSV_Handle{ 0 }),
								create_shader_arguments(*arena, vs_cb_hnds, vs_srv_hnds, ps_cb_hnds, ps_srv_hnds),
								nil_buff_hnd,
								0,
								D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
								hnd_submesh
							);
						}

						cmd_list.copy_texture(hnd_skybox_map_rtv, registry.get<Render_RTV_Handle>(environment_map_rts[0]));

					};
			}
		);
		

		render_graph.add_pass_callback(L"Convolve Cubemap", [&](Render_Graph_Builder& builder)
			{
				auto& env_map_data = scope.get<Environment_Map_Data>();

				auto env_map_srv = builder.read(env_map_data.environment_map, Bind_Flag::SHADER_RESOURCE, 0, 6);

				Render_Rect viewport;
				viewport.top_left_x = 0;
				viewport.top_left_y = 0;
				viewport.width = 64;
				viewport.height = 64;

				Render_Graph_Resource_Desc irradiance_map_desc;
				irradiance_map_desc.arr_size = 6;
				irradiance_map_desc.format = Format::FORMAT_R11G11B10_FLOAT;
				irradiance_map_desc.width = viewport.width;
				irradiance_map_desc.height = viewport.height;
				irradiance_map_desc.is_cubemap = true;

				Render_Graph_Resource_Desc pre_filter_env_map_desc;
				pre_filter_env_map_desc.arr_size = 6;
				pre_filter_env_map_desc.format = Format::FORMAT_R11G11B10_FLOAT;
				pre_filter_env_map_desc.width = viewport.width;
				pre_filter_env_map_desc.height = viewport.height;
				pre_filter_env_map_desc.is_cubemap = true;
				pre_filter_env_map_desc.has_mips = true;

				Render_Graph_Resource_Desc brdf_lut_tex_desc;
				brdf_lut_tex_desc.format = Format::FORMAT_R11G11B10_FLOAT;
				brdf_lut_tex_desc.width = 512;
				brdf_lut_tex_desc.height = 512;

				int mip_levels = 0;
				for (int start_width = pre_filter_env_map_desc.width; start_width >= 1; start_width /= 2)
				{
					mip_levels += 1;
				}

				Irradiance_Data irradiance_data;
				auto irradiance_map = irradiance_data.irradiance_map = builder.create(irradiance_map_desc);
				auto pre_filtered_env_map = irradiance_data.pre_filtered_env_map = builder.create(pre_filter_env_map_desc);
				auto brdf_lut_map = irradiance_data.brdf_lut_map = builder.create(brdf_lut_tex_desc);

				Render_Graph_Resource irradiance_map_rts[6];
				Render_Graph_Resource pre_filtered_env_map_rts[10][6];
				Render_Graph_Resource brdf_lut_map_rt;
				zassert(mip_levels < 10);
				
				for (size_t i = 0; i < 6; i++)
				{
					irradiance_map_rts[i] = builder.write(irradiance_map, Bind_Flag::RENDER_TARGET, i, 1);
					builder.read(irradiance_map_rts[i], Bind_Flag::SHADER_RESOURCE, i, 1);

					for (int mip = 0; mip < mip_levels; mip++)
					{
						pre_filtered_env_map_rts[mip][i] = builder.write(pre_filtered_env_map, Bind_Flag::RENDER_TARGET, i, 1, mip);
						builder.read(pre_filtered_env_map_rts[mip][i], Bind_Flag::SHADER_RESOURCE, i, 1);
					}
				}

				brdf_lut_map_rt = builder.write(brdf_lut_map, Bind_Flag::RENDER_TARGET);
				builder.read(brdf_lut_map_rt , Bind_Flag::SHADER_RESOURCE);

				scope.set(irradiance_data);


				return [=](Render_Command_List& cmd_list, Render_Graph_Registry& registry)
					{
						struct ibl_draw_data
						{
							float roughness{ 0.0f };
							u32 is_brdf_lut_pass{ false };
						};

						ibl_draw_data data;
						cmd_list.update_buffer(*arena, hnd_ibl_spec_draw_cb, &data, sizeof(data));

						auto matrix_identity = DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity());
						auto proj_mat = dx::XMMatrixTranspose(dx::XMMatrixPerspectiveFovLH(dx::XM_PIDIV2, 1.0f, 100.0f, 0.1f));
						cmd_list.update_buffer(*arena, hnd_world_cb, &matrix_identity, sizeof(matrix_identity));
						cmd_list.update_buffer(*arena, hnd_proj_cb, &proj_mat, sizeof(proj_mat));

						//TODO: create default set of these view matrices
						DirectX::XMMATRIX view_matrices[6];
						view_matrices[0] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(dx::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
						view_matrices[1] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(dx::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
						view_matrices[2] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(dx::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f)));
						view_matrices[3] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(dx::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f)));
						view_matrices[4] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(dx::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
						view_matrices[5] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(dx::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));


						for (size_t mip = 0; mip < mip_levels; mip++)
						{
							data.roughness = mip * (1.0f / mip_levels);
							cmd_list.update_buffer(*arena, hnd_ibl_spec_draw_cb, &data, sizeof(data));

							Render_Rect mip_viewport;
							mip_viewport.top_left_x = mip_viewport.top_left_y = 0;
							mip_viewport.height = mip_viewport.width = viewport.width / std::pow(2, mip);
							cmd_list.set_viewport(mip_viewport);

							for (size_t i = 0; i < 6; i++)
							{
								cmd_list.update_buffer(*arena, hnd_view_cb, &view_matrices[i], sizeof(view_matrices[i]));

								//convolve_cubemap_face(arena, cmd_list, registry, env_map_srv, pre_filtered_env_map_rts[mip][i], hnd_world_cb, hnd_view_cb, hnd_ibl_spec_draw_cb, hnd_proj_cb);
								Render_RTV_Handle rts[] = { registry.get<Render_RTV_Handle>(pre_filtered_env_map_rts[mip][i]) };

								Constant_Buffer_Handle vs_cb_hnds[] = { hnd_world_cb, hnd_view_cb, hnd_proj_cb };
								Render_SRV_Handle vs_srv_hnds[] = { 0 };
								Constant_Buffer_Handle ps_cb_hnds[] = { hnd_ibl_spec_draw_cb };
								Render_SRV_Handle ps_srv_hnds[] = { registry.get<Render_SRV_Handle>(env_map_srv) };
								Buffer_Handle_t* nil_buff_hnd = {};
								Submesh_Handle_t hnd_submesh;
								hnd_submesh.num_vertices = 36;

								cmd_list.draw(
									pipeline_state_manager.get(Pipeline_State::BUILD_PRE_FILTERED_MAP),
									create_shader_render_targets(*arena, rts, 1, Render_DSV_Handle{ 0 }),
									create_shader_arguments(*arena, vs_cb_hnds, vs_srv_hnds, ps_cb_hnds, ps_srv_hnds),
									nil_buff_hnd,
									0,
									D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
									hnd_submesh
								);
							}
						}
						cmd_list.copy_texture(hnd_prefiltered_env_map_rtv, registry.get<Render_RTV_Handle>(pre_filtered_env_map_rts[0][0]));


						cmd_list.set_viewport(viewport);

						for (size_t i = 0; i < 6; i++)
						{
							cmd_list.update_buffer(*arena, hnd_view_cb, &view_matrices[i], sizeof(view_matrices[i]));

							//convolve_cubemap_face(arena, cmd_list, registry, env_map_srv, irradiance_map_rts[i], hnd_world_cb, hnd_view_cb, hnd_ibl_spec_draw_cb, hnd_proj_cb);
							Render_RTV_Handle rts[] = { registry.get<Render_RTV_Handle>(irradiance_map_rts[i]) };

							Constant_Buffer_Handle vs_cb_hnds[] = { hnd_world_cb, hnd_view_cb, hnd_proj_cb };
							Render_SRV_Handle vs_srv_hnds[] = { 0 };
							Constant_Buffer_Handle ps_cb_hnds[] = { hnd_ibl_spec_draw_cb };
							Render_SRV_Handle ps_srv_hnds[] = { registry.get<Render_SRV_Handle>(env_map_srv) };
							Buffer_Handle_t* nil_buff_hnd = {};
							Submesh_Handle_t hnd_submesh;
							hnd_submesh.num_vertices = 36;

							cmd_list.draw(
								pipeline_state_manager.get(Pipeline_State::CONVOLVE_CUBEMAP),
								create_shader_render_targets(*arena, rts, 1, Render_DSV_Handle{ 0 }),
								create_shader_arguments(*arena, vs_cb_hnds, vs_srv_hnds, ps_cb_hnds, ps_srv_hnds),
								nil_buff_hnd,
								0,
								D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
								hnd_submesh
							);
						}

						cmd_list.copy_texture(hnd_irradiance_map_rtv, registry.get<Render_RTV_Handle>(irradiance_map_rts[0]));


						data.is_brdf_lut_pass = true;

						cmd_list.update_buffer(*arena, hnd_ibl_spec_draw_cb, &data, sizeof(data));
						cmd_list.set_viewport(Render_Rect{ 0, 0, brdf_lut_tex_desc.width, brdf_lut_tex_desc.height });

						Render_RTV_Handle rts[] = { registry.get<Render_RTV_Handle>(brdf_lut_map_rt) };

						Constant_Buffer_Handle vs_cb_hnds[] = { 0 };
						Render_SRV_Handle vs_srv_hnds[] = { 0 };
						Constant_Buffer_Handle ps_cb_hnds[] = { hnd_ibl_spec_draw_cb };
						Render_SRV_Handle ps_srv_hnds[] = { 0/*registry.get<Render_SRV_Handle>(src_map)*/ };
						Buffer_Handle_t* nil_buff_hnd = {};
						Submesh_Handle_t hnd_submesh;
						hnd_submesh.num_vertices = 4;

						cmd_list.draw(
							pipeline_state_manager.get(Pipeline_State::BUILD_BRDF_LUT_MAP),
							create_shader_render_targets(*arena, rts, 1, Render_DSV_Handle{ 0 }),
							create_shader_arguments(*arena, vs_cb_hnds, vs_srv_hnds, ps_cb_hnds, ps_srv_hnds),
							nil_buff_hnd,
							0,
							D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
							hnd_submesh
						);

						cmd_list.copy_texture(hnd_brdf_lut_map_rtv, registry.get<Render_RTV_Handle>(brdf_lut_map_rt));


						cmd_list.set_viewport(Render_Rect{ 0,0,1920,1080 });

					};
			}
		);
	}

	void convolve_cubemap_face(Arena* arena, Render_Command_List& cmd_list, Render_Graph_Registry& registry, const Render_Graph_Resource& src_map, const Render_Graph_Resource& dest_face_rtv,
		Constant_Buffer_Handle hnd_world_cb, Constant_Buffer_Handle hnd_view_cb, Constant_Buffer_Handle hnd_ibl_spec_draw_cb, Constant_Buffer_Handle hnd_proj_cb)
	{
		Render_RTV_Handle rts[] = { registry.get<Render_RTV_Handle>(dest_face_rtv) };

		Constant_Buffer_Handle vs_cb_hnds[] = { hnd_world_cb, hnd_view_cb, hnd_proj_cb };
		Render_SRV_Handle vs_srv_hnds[] = { 0 };
		Constant_Buffer_Handle ps_cb_hnds[] = { hnd_ibl_spec_draw_cb };
		Render_SRV_Handle ps_srv_hnds[] = { registry.get<Render_SRV_Handle>(src_map) };
		Buffer_Handle_t* nil_buff_hnd = {};
		Submesh_Handle_t hnd_submesh;
		hnd_submesh.num_vertices = 36;

		cmd_list.draw(
			pipeline_state_manager.get(Pipeline_State::CONVOLVE_CUBEMAP),
			create_shader_render_targets(*arena, rts, 1, Render_DSV_Handle{ 0 }),
			create_shader_arguments(*arena, vs_cb_hnds, vs_srv_hnds, ps_cb_hnds, ps_srv_hnds),
			nil_buff_hnd,
			0,
			D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
			hnd_submesh
		);
	}

}