#include "SSSPass.h"

#include "renderer/frontend/PipelineStateManager.h"

#include "renderer/backend/GraphicsContext.h"
#include "renderer/passes/LightingPass.h"
#include "renderer/passes/GBufferPass.h"

namespace zorya
{
	SSS_Pass::SSS_Pass(Render_Graph& render_graph, Render_Scope& scope, Arena* arena, Constant_Buffer_Handle m_hnd_frame_cb, Constant_Buffer_Handle m_hnd_object_cb)
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





								//	//	if (sss_type_in_scene[(uint32_t)SSS_MODEL::JIMENEZ_SEPARABLE])
		//	//	{
		//	//		rhi.m_gfx_context->m_annot->BeginEvent(L"Jimenez Separable SSS");
		//	//		{
		//	//			Render_RTV_Handle render_target_hnds_sep_sss_pass[] = { hnd_skin_rtv[1], hnd_skin_rtv[4] };

		//	//			dx::XMFLOAT4 clear_col = { 0.0f,0.0f,0.0f,1.0f };
		//	//			rhi.m_gfx_context->clear_rtv(render_target_hnds_sep_sss_pass[0], clear_col);

		//	//			rhi.m_gfx_context->set_pixel_shader(&m_sssss_pixel_shader);

		//	//			uint32_t val = (uint32_t)SSS_MODEL::JIMENEZ_SEPARABLE;
		//	//			bool suc = set_constant_buff_var(cb_data, "sssModelId", &val, sizeof(val));
		//	//			assert(suc);

		//	//			rhi.m_gfx_context->m_annot->BeginEvent(L"Horizontal Pass");
		//	//			{
		//	//				rhi.m_gfx_context->set_ps_resource(null_srv_hnds, 0, 1);
		//	//				rhi.m_gfx_context->set_ps_resource(null_srv_hnds, 2, 1);
		//	//				rhi.m_gfx_context->set_render_targets(render_target_hnds_sep_sss_pass, Render_DSV_Handle{ 0 }, 2);

		//	//				//mat.mat_prms.dir = dx::XMFLOAT2(1.0f, 0.0f);
		//	//				float val[2] = { 1.0f,0.0f };
		//	//				bool suc = set_constant_buff_var(cb_data, "dir", &val, sizeof(val));
		//	//				assert(suc);
		//	//				//rhi.m_device.update_constant_buffer(m_hnd_object_cb, Object_Constant_Buff{ mat.mat_prms });
		//	//				rhi.m_gfx_context->update_subresource(cb_resource);

		//	//				rhi.m_gfx_context->set_ps_resource(&hnd_skin_srv[3], 0, 1);
		//	//				rhi.m_gfx_context->set_ps_resource(&hnd_gbuff_srv[G_Buffer::ROUGH_MET], 1, 1);
		//	//				rhi.m_gfx_context->set_ps_resource(&hnd_skin_srv[2], 2, 1);
		//	//				rhi.m_gfx_context->set_ps_resource(&hnd_depth_srv, 3, 1);
		//	//				rhi.m_gfx_context->set_ps_resource(&hnd_gbuff_srv[G_Buffer::ALBEDO], 4, 1);
		//	//				rhi.m_gfx_context->set_ps_resource(null_srv_hnds, 5, 1);

		//	//				rhi.m_gfx_context->draw_full_quad();
		//	//			}
		//	//			rhi.m_gfx_context->m_annot->EndEvent();

		//	//			render_target_hnds_sep_sss_pass[0] = hnd_skin_rtv[0];

		//	//			rhi.m_gfx_context->m_annot->BeginEvent(L"Vertical Pass");
		//	//			{
		//	//				//mat.mat_prms.dir = dx::XMFLOAT2(0.0f, 1.0f);
		//	//				//rhi.m_device.update_constant_buffer(m_hnd_object_cb, Object_Constant_Buff{ mat.mat_prms });
		//	//				float val[2] = { 0.0f, 1.0f };
		//	//				bool suc = set_constant_buff_var(cb_data, "dir", &val, sizeof(val));
		//	//				assert(suc);
		//	//				//rhi.m_device.update_constant_buffer(m_hnd_object_cb, Object_Constant_Buff{ mat.mat_prms });
		//	//				rhi.m_gfx_context->update_subresource(cb_resource);

		//	//				rhi.m_gfx_context->set_render_targets(render_target_hnds_sep_sss_pass, Render_DSV_Handle{ 0 }, 2);
		//	//				rhi.m_gfx_context->set_ps_resource(&hnd_skin_srv[1], 0, 1);

		//	//				rhi.m_gfx_context->draw_full_quad();
		//	//			}
		//	//			rhi.m_gfx_context->m_annot->EndEvent();
		//	//		}
		//	//		rhi.m_gfx_context->m_annot->EndEvent();
		//	//	}

		//	//	if (sss_type_in_scene[(uint32_t)SSS_MODEL::JIMENEZ_GAUSS])
		//	//	{
		//	//		rhi.m_gfx_context->m_annot->BeginEvent(L"Jimenez Sum of Gaussians SSS");
		//	//		{
		//	//			rhi.m_gfx_context->set_pixel_shader(&m_sssss_pixel_shader);

		//	//			//TODO: change m_final_... with other render target (m_final is SRGB)
		//	//			//ID3D11RenderTargetView* render_targets_gauss_sss_pass[4] = {
		//	//			//	m_skin_rt[5],
		//	//			//	m_skin_rt[1],

		//	//			//	m_skin_rt[3],
		//	//			//	m_skin_rt[0]
		//	//			//};

		//	//			Render_RTV_Handle render_target_hnds_gauss_sss_pass[4] = {
		//	//				hnd_skin_rtv[5],
		//	//				hnd_skin_rtv[1],

		//	//				hnd_skin_rtv[3],
		//	//				hnd_skin_rtv[0]
		//	//			};

		//	//			//float clear_col[4] = { 0.0f,0.0f,0.0f,1.0f };
		//	//			//rhi.m_context->ClearRenderTargetView(render_targets_gauss_sss_pass[3], clear_col);

		//	//			float blend_factors[4][4] = {
		//	//				{1.0f, 1.0f, 1.0f, 1.0f},
		//	//				{0.3251f, 0.45f, 0.3583f, 1.0f},
		//	//				{0.34f, 0.1864f, 0.0f, 1.0f},
		//	//				{0.46f, 0.0f, 0.0402f, 1.0f}
		//	//			};

		//	//			float gauss_standard_deviations[4] = { 0.08f, 0.2126f, 0.4694f, 1.3169f };

		//	//			//mat.mat_prms.sss_model_id = (uint32_t)SSS_MODEL::JIMENEZ_GAUSS;
		//	//			uint32_t val = (uint32_t)SSS_MODEL::JIMENEZ_GAUSS;
		//	//			bool suc = set_constant_buff_var(cb_data, "sssModelId", &val, sizeof(val));
		//	//			assert(suc);

		//	//			for (int i = 0; i < 4; i++)
		//	//			{
		//	//				//mat.mat_prms.sd_gauss_jim = gauss_standard_deviations[i];
		//	//				float val = gauss_standard_deviations[i];
		//	//				bool suc = set_constant_buff_var(cb_data, "sd_gauss_jim", &val, sizeof(val));
		//	//				assert(suc);

		//	//				rhi.m_gfx_context->m_annot->BeginEvent(L"Horizontal Pass");
		//	//				{
		//	//					rhi.m_gfx_context->set_ps_resource(null_srv_hnds, 0, 1);
		//	//					rhi.m_gfx_context->set_ps_resource(null_srv_hnds, 2, 1);

		//	//					rhi.m_gfx_context->set_render_targets(render_target_hnds_gauss_sss_pass, Render_DSV_Handle{ 0 }, 2);

		//	//					//mat.mat_prms.dir = dx::XMFLOAT2(1.0f, 0.0f);
		//	//					float val[2] = { 1.0f, 0.0f };
		//	//					bool suc = set_constant_buff_var(cb_data, "dir", &val, sizeof(val));
		//	//					assert(suc);
		//	//					//rhi.m_device.update_constant_buffer(m_hnd_object_cb, Object_Constant_Buff{ mat.mat_prms });
		//	//					rhi.m_gfx_context->update_subresource(cb_resource);

		//	//					rhi.m_gfx_context->m_context->OMSetBlendState(NULL, 0, D3D11_DEFAULT_SAMPLE_MASK);

		//	//					rhi.m_gfx_context->set_ps_resource(i == 0 ? &hnd_skin_srv[3] : &hnd_skin_srv[0], 0, 1);
		//	//					rhi.m_gfx_context->set_ps_resource(&hnd_gbuff_srv[G_Buffer::ROUGH_MET], 1, 1);
		//	//					rhi.m_gfx_context->set_ps_resource(&hnd_skin_srv[2], 2, 1);
		//	//					rhi.m_gfx_context->set_ps_resource(&hnd_depth_srv, 3, 1);
		//	//					rhi.m_gfx_context->set_ps_resource(&hnd_gbuff_srv[G_Buffer::ALBEDO], 4, 1);
		//	//					rhi.m_gfx_context->set_ps_resource(null_srv_hnds, 5, 1);

		//	//					rhi.m_gfx_context->draw_full_quad();
		//	//				}
		//	//				rhi.m_gfx_context->m_annot->EndEvent();


		//	//				rhi.m_gfx_context->m_annot->BeginEvent(L"Vertical Pass");
		//	//				{
		//	//					rhi.m_gfx_context->set_ps_resource(null_srv_hnds, 0, 1);

		//	//					rhi.m_gfx_context->set_render_targets(&render_target_hnds_gauss_sss_pass[2], Render_DSV_Handle{ 0 }, 2);

		//	//					//mat.mat_prms.dir = dx::XMFLOAT2(0.0f, 1.0f);
		//	//					float val[2] = { 0.0f, 1.0f };
		//	//					bool suc = set_constant_buff_var(cb_data, "dir", &val, sizeof(val));
		//	//					assert(suc);
		//	//					//rhi.m_device.update_constant_buffer(m_hnd_object_cb, Object_Constant_Buff{mat.mat_prms});
		//	//					rhi.m_gfx_context->update_subresource(cb_resource);

		//	//					rhi.m_gfx_context->m_context->OMSetBlendState(rhi.get_blend_state(1), blend_factors[i], D3D11_DEFAULT_SAMPLE_MASK);

		//	//					rhi.m_gfx_context->set_ps_resource(&hnd_skin_srv[1], 0, 1);

		//	//					rhi.m_gfx_context->draw_full_quad();
		//	//				}
		//	//				rhi.m_gfx_context->m_annot->EndEvent();

		//	//			}

		//	//			rhi.m_gfx_context->m_context->OMSetBlendState(NULL, 0, D3D11_DEFAULT_SAMPLE_MASK);
		//	//		}
		//	//		rhi.m_gfx_context->m_annot->EndEvent();
		//	//	}
						
					};
			}
		);
	}
}
