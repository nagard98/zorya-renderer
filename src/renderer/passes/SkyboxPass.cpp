#include "SkyboxPass.h"
#include "utils/Arena.h"
#include "renderer/frontend/Shader.h"
#include "renderer/frontend/PipelineStateManager.h"
#include "renderer/passes/PresentPass.h"
#include "renderer/passes/ShadowMappingPass.h"
#include "renderer/passes/EquirectangularToCubemapPass.h"

namespace zorya
{
	Skybox_Pass::Skybox_Pass(Render_Graph& render_graph, Render_Scope& scope, Render_SRV_Handle hnd_sky_cubemap_srv, const View_Desc& view_desc, Arena* arena,
		Constant_Buffer_Handle hnd_world_cb, Constant_Buffer_Handle hnd_view_cb, Constant_Buffer_Handle hnd_proj_cb, Render_Rect& scene_viewport)
	{
		struct World_CB
		{
			dx::XMMATRIX world_matrix;
		};

		struct View_CB
		{
			dx::XMMATRIX view_matrix;
		};

		struct Proj_CB
		{
			dx::XMMATRIX proj_matrix;
		};

		render_graph.add_pass_callback(L"Skybox Pass", [&](Render_Graph_Builder& builder)
			{
				Render_Graph_Resource_Desc skybox_desc;
				skybox_desc.height = 1080;
				skybox_desc.width = 1920;
				skybox_desc.format = Format::FORMAT_R11G11B10_FLOAT;

				auto skybox = builder.create(skybox_desc);
				skybox = builder.write(skybox, Bind_Flag::RENDER_TARGET);
				scope.set(Skybox_Data{ skybox });

				auto& final_texture = scope.get<Final_Render_Graph_Texture>();

				//auto& irradiance_data = scope.get<Irradiance_Data>();
				//auto environment_map = builder.read(irradiance_data.irradiance_map, Bind_Flag::SHADER_RESOURCE, 0, 6);

				//TODO: temporary
				//auto& shadow_mapping_data = scope.get<Shadow_Mapping_Data>();
				//builder.read(shadow_mapping_data.dir_shadowmap, Bind_Flag::DEPTH_STENCIL);

				return [=](Render_Command_List& cmd_list, Render_Graph_Registry& registry)
					{
						cmd_list.set_viewport(scene_viewport);

						//auto final_tex = final_texture.texture;
						auto final_tex = registry.get<Render_RTV_Handle>(skybox);

						//Using cam rotation matrix as view, to ignore cam translation, making skybox always centered
						World_CB tmpOCB{ dx::XMMatrixIdentity() };
						View_CB tmpVCB{ dx::XMMatrixTranspose(view_desc.cam.get_rotation_matrix()) };
						Proj_CB tmpPCB{ view_desc.cam.get_proj_matrix_transposed() };

						cmd_list.update_buffer(*arena, hnd_world_cb, &tmpOCB, sizeof(tmpOCB));
						cmd_list.update_buffer(*arena, hnd_view_cb, &tmpVCB, sizeof(tmpVCB));
						cmd_list.update_buffer(*arena, hnd_proj_cb, &tmpPCB, sizeof(tmpPCB));

						Constant_Buffer_Handle vs_cb_hnds[] = { hnd_world_cb, hnd_view_cb, hnd_proj_cb };
						Render_SRV_Handle vs_srv_hnds[] = { 0 };
						Constant_Buffer_Handle ps_cb_hnds[] = { 0 };
						Render_SRV_Handle ps_srv_hnds[] = { hnd_sky_cubemap_srv };
						Buffer_Handle_t* nil_buff_hnd = {};
						Submesh_Handle_t hnd_submesh;
						hnd_submesh.num_vertices = 36;

						cmd_list.draw(
							pipeline_state_manager.get(Pipeline_State::SKYBOX),
							create_shader_render_targets(*arena, &final_tex, 1, Render_DSV_Handle{ 0 }),
							create_shader_arguments(*arena, vs_cb_hnds, vs_srv_hnds, ps_cb_hnds, ps_srv_hnds),
							nil_buff_hnd,
							0,
							D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
							hnd_submesh
						);
			
					};
			}
		);
	}
}