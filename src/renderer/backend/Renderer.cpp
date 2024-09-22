#include "renderer/backend/Renderer.h"
#include "renderer/backend/BufferCache.h"
#include "renderer/backend/ResourceCache.h"
#include "renderer/backend/JimenezSeparable.h"
#include "renderer/backend/RenderGraph.h"
#include "renderer/backend/PipelineStateObject.h"

#include "renderer/backend/rhi/RenderHardwareInterface.h"

#include "renderer/passes/GBufferPass.h"
#include "renderer/passes/SkyboxPass.h"
#include "renderer/passes/PresentPass.h"
#include "renderer/passes/LightingPass.h"
#include "renderer/passes/DebugViewPass.h"
#include "renderer/passes/ShadowMappingPass.h"
#include "renderer/passes/SSSPass.h"

#include "renderer/frontend/SceneManager.h"
#include "renderer/frontend/Material.h"
#include "renderer/frontend/Lights.h"
#include "renderer/frontend/Camera.h"
#include "renderer/frontend/Shader.h"
#include "utils/Arena.h"

#include "ApplicationConfig.h"

#include <DDSTextureLoader.h>

#include <d3d11_1.h>
#include <vector>
#include <array>
#include <cassert>


namespace zorya
{
	Renderer renderer;

	Renderer::Renderer() {}
	Renderer::~Renderer() {}

	void Renderer::release_all_resources() {}

	HRESULT Renderer::init(bool reset)
	{
		if (reset) rhi.m_device.release_all_resources();

		HRESULT hr;

		m_scene_viewport.top_left_x = 0.0f;
		m_scene_viewport.top_left_y = 0.0f;
		m_scene_viewport.width = g_resolutionWidth;
		m_scene_viewport.height = g_resolutionHeight;

		m_shadow_map_viewport.top_left_x = 0.0f;
		m_shadow_map_viewport.top_left_y = 0.0f;
		m_shadow_map_viewport.width = 4096.0f;
		m_shadow_map_viewport.height = 4096.f;

		D3D11_BUFFER_DESC material_cb_desc;
		ZeroMemory(&material_cb_desc, sizeof(material_cb_desc));
		material_cb_desc.ByteWidth = sizeof(Material_Params);
		material_cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		material_cb_desc.Usage = D3D11_USAGE_DEFAULT;

		RETURN_IF_FAILED2(hr, rhi.create_constant_buffer(&m_hnd_object_cb, &material_cb_desc).value);


		//World transform constant buffer setup---------------------------------------------------
		D3D11_BUFFER_DESC cb_world_desc;
		cb_world_desc.Usage = D3D11_USAGE_DEFAULT;
		cb_world_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cb_world_desc.CPUAccessFlags = 0;
		cb_world_desc.MiscFlags = 0;
		cb_world_desc.ByteWidth = sizeof(World_CB);

		RETURN_IF_FAILED2(hr, rhi.create_constant_buffer(&hnd_world_cb, &cb_world_desc).value);
				
		//---------------------------------------------------

		D3D11_BUFFER_DESC cb_cam_transf_desc;
		cb_cam_transf_desc.Usage = D3D11_USAGE_DEFAULT;
		cb_cam_transf_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cb_cam_transf_desc.CPUAccessFlags = 0;
		cb_cam_transf_desc.MiscFlags = 0;
		cb_cam_transf_desc.ByteWidth = sizeof(Cam_Transformation);

		RETURN_IF_FAILED2(hr, rhi.create_constant_buffer(&hnd_cam_transf_cb, &cb_cam_transf_desc).value);

		D3D11_BUFFER_DESC cb_light_draw_desc;
		cb_light_draw_desc.Usage = D3D11_USAGE_DEFAULT;
		cb_light_draw_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cb_light_draw_desc.CPUAccessFlags = 0;
		cb_light_draw_desc.MiscFlags = 0;
		cb_light_draw_desc.ByteWidth = sizeof(Light_Draw_Constants);

		RETURN_IF_FAILED2(hr, rhi.create_constant_buffer(&hnd_light_draw_cb, &cb_light_draw_desc).value);

		//View matrix constant buffer setup-------------------------------------------------------------
		D3D11_BUFFER_DESC cb_cam_desc;
		cb_cam_desc.Usage = D3D11_USAGE_DEFAULT;
		cb_cam_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cb_cam_desc.CPUAccessFlags = 0;
		cb_cam_desc.MiscFlags = 0;
		cb_cam_desc.ByteWidth = sizeof(View_CB);

		RETURN_IF_FAILED2(hr, rhi.create_constant_buffer(&hnd_view_cb, &cb_cam_desc).value);

		rhi.m_gfx_context->set_vs_constant_buff(&hnd_view_cb, 1, 1);
		//----------------------------------------------------------------

		//Projection matrix constant buffer setup--------------------------------------
		D3D11_BUFFER_DESC cb_proj_desc;
		cb_proj_desc.Usage = D3D11_USAGE_DEFAULT;
		cb_proj_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cb_proj_desc.CPUAccessFlags = 0;
		cb_proj_desc.MiscFlags = 0;
		cb_proj_desc.ByteWidth = sizeof(Proj_CB);

		RETURN_IF_FAILED2(hr, rhi.create_constant_buffer(&hnd_proj_cb, &cb_proj_desc).value);

		rhi.m_gfx_context->set_vs_constant_buff(&hnd_proj_cb, 2, 1);
		//------------------------------------------------------------------

		//Light constant buffer setup---------------------------------------
		D3D11_BUFFER_DESC light_buff_desc;
		ZeroMemory(&light_buff_desc, sizeof(light_buff_desc));
		light_buff_desc.Usage = D3D11_USAGE_DEFAULT;
		light_buff_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		light_buff_desc.ByteWidth = sizeof(Frame_Constant_Buff);

		RETURN_IF_FAILED2(hr, rhi.create_constant_buffer(&m_hnd_frame_cb, &light_buff_desc).value);

		//---------------------------------------------------------

		D3D11_BUFFER_DESC inv_matrix_cb_desc;
		ZeroMemory(&inv_matrix_cb_desc, sizeof(inv_matrix_cb_desc));
		inv_matrix_cb_desc.ByteWidth = sizeof(dx::XMMatrixIdentity()) * 2;
		inv_matrix_cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		inv_matrix_cb_desc.Usage = D3D11_USAGE_DEFAULT;

		RETURN_IF_FAILED2(hr, rhi.create_constant_buffer(&hnd_inv_mat_cb, &inv_matrix_cb_desc).value);

		//---------------------------------------------------------

		D3D11_BUFFER_DESC dir_shadow_map_buff_desc;
		ZeroMemory(&dir_shadow_map_buff_desc, sizeof(D3D11_BUFFER_DESC));
		dir_shadow_map_buff_desc.Usage = D3D11_USAGE_DEFAULT;
		dir_shadow_map_buff_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		dir_shadow_map_buff_desc.ByteWidth = sizeof(Dir_Shadow_CB);

		RETURN_IF_FAILED2(hr, rhi.create_constant_buffer(&hnd_dir_shad_cb, &dir_shadow_map_buff_desc).value);

		D3D11_BUFFER_DESC omni_dir_shadow_map_buff_desc;
		ZeroMemory(&omni_dir_shadow_map_buff_desc, sizeof(D3D11_BUFFER_DESC));
		omni_dir_shadow_map_buff_desc.Usage = D3D11_USAGE_DEFAULT;
		omni_dir_shadow_map_buff_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		omni_dir_shadow_map_buff_desc.ByteWidth = sizeof(Omni_Dir_Shadow_CB);

		RETURN_IF_FAILED2(hr, rhi.create_constant_buffer(&hnd_omni_dir_shad_cb, &omni_dir_shadow_map_buff_desc).value);


		////-----------------------------------------------------------
		ZRY_Usage def{ D3D11_USAGE_DEFAULT };

		RETURN_IF_FAILED2(hr, rhi.create_tex_2d(&hnd_final_rt, nullptr, def, ZRY_Bind_Flags{ D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE }, ZRY_Format{ DXGI_FORMAT_R8G8B8A8_TYPELESS }, g_resolutionWidth, g_resolutionHeight, 1, nullptr, nullptr, false, 1).value);
		RETURN_IF_FAILED2(hr, rhi.create_srv_tex_2d(&hnd_final_srv, &hnd_final_rt, ZRY_Format{ DXGI_FORMAT_R8G8B8A8_UNORM }).value);
		RETURN_IF_FAILED2(hr, rhi.create_rtv_tex_2d(&hnd_final_rtv, &hnd_final_rt, ZRY_Format{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB }).value);

		//----------------------------Skybox----------------------
		wrl::ComPtr<ID3D11Resource> sky_texture;
		ID3D11ShaderResourceView* m_cubemap_view = nullptr;
		hr = dx::CreateDDSTextureFromFileEx(rhi.m_device.m_device, L"./assets/skybox.dds", 0, D3D11_USAGE_DEFAULT,
			D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_TEXTURECUBE, dx::DX11::DDS_LOADER_DEFAULT, sky_texture.GetAddressOf(), &m_cubemap_view);
		RETURN_IF_FAILED(hr);

		hnd_sky_cubemap_srv = rhi.add_srv(std::move(m_cubemap_view));

		std::vector<float> krn;
		//load_kernel_file("./assets/Skin1_PreInt_DISCSEP.bn", krn);

		//override_ssss_discr_sep_kernel(krn);
		separable_sss_calculate_kernel();

		return hr;
	}


	void Renderer::shutdown()
	{
		release_all_resources();
	}


	static void setup_scene_lights(const View_Desc& view_desc, Scene_Lights& scene_lights, Dir_Shadow_CB& dir_shadow_cb, Omni_Dir_Shadow_CB& omni_dir_shad_cb);


	void Renderer::render_view(const View_Desc& view_desc)
	{
		static Render_Command_List cmd_list(rhi.m_device);
		static Arena arena(10 * 1024 * 1024);
		static Render_Graph render_graph;

		Render_Scope scope;

		render_graph.import_persistent_resource<Final_Render_Graph_Texture>(scope, hnd_final_rtv);

		View_CB tmpVCB{ view_desc.cam.get_view_matrix_transposed() };
		Proj_CB tmpPCB{ view_desc.cam.get_proj_matrix_transposed() };
		Scene_Lights tmp_lights_cb;
		Dir_Shadow_CB dir_shadow_cb{};
		Omni_Dir_Shadow_CB omni_dir_shadow_cb{};		

		setup_scene_lights(view_desc, tmp_lights_cb, dir_shadow_cb, omni_dir_shadow_cb);

		auto frame_cb = Frame_Constant_Buff{ tmp_lights_cb };
		for (size_t i = 0; i < view_desc.sss_profiles.size(); i++)
		{
			Diffusion_Profile_Handle hnd = view_desc.sss_profiles[i];
			frame_cb.sss_params[i] = resource_cache.m_diffusion_profiles[hnd.index].sss_params;
		}

		cmd_list.update_buffer(arena, m_hnd_frame_cb, &frame_cb, sizeof(frame_cb));
		cmd_list.update_buffer(arena, hnd_view_cb, &tmpVCB, sizeof(tmpVCB));
		cmd_list.update_buffer(arena, hnd_proj_cb, &tmpPCB, sizeof(tmpPCB));

		Shadow_Mapping_Pass(render_graph, scope, &arena, view_desc, hnd_world_cb, hnd_omni_dir_shad_cb, omni_dir_shadow_cb, m_shadow_map_viewport, hnd_light_draw_cb);
		Skybox_Pass(render_graph, scope, hnd_sky_cubemap_srv, view_desc, &arena, hnd_world_cb, hnd_view_cb, hnd_proj_cb, m_scene_viewport);
		GBuffer_Pass(render_graph, scope, &arena, view_desc, hnd_world_cb, hnd_view_cb, hnd_proj_cb);
		Lighting_Pass(render_graph, scope, &arena, view_desc, hnd_inv_mat_cb, m_hnd_frame_cb, hnd_omni_dir_shad_cb, hnd_cam_transf_cb, hnd_light_draw_cb);
		SSS_Pass(render_graph, scope, &arena, m_hnd_frame_cb, m_hnd_object_cb);
		Debug_View_Pass(render_graph, scope);
		Present_Pass(render_graph, scope, &arena);

		render_graph.compile();
		render_graph.execute(cmd_list);

		cmd_list.clear();
		arena.clear();
	}


	static void setup_scene_lights(const View_Desc& view_desc, Scene_Lights& scene_lights, Dir_Shadow_CB& dir_shadow_cb, Omni_Dir_Shadow_CB& omni_dir_shad_cb)
	{
		int i = 0;

		for (int dir_light_idx = 0; i < view_desc.num_dir_lights; i++, dir_light_idx++)
		{
			const Directional_Light& dir_light = view_desc.lights_info.at(i).dir_light; //dir_lights.at(i).dir_light;
			scene_lights.dir_light = dir_light;

			dx::XMVECTOR transformed_dir_light = dx::XMVector4Transform(dx::XMLoadFloat4(&dir_light.dir), view_desc.lights_info.at(i).final_world_transform);
			dx::XMStoreFloat4(&scene_lights.dir_light.dir, dx::XMVector4Transform(transformed_dir_light, view_desc.cam.get_view_matrix()));

			//Setup for shadowmapping
			dx::XMVECTOR dir_light_pos = dx::XMVectorMultiply(dx::XMVectorNegate(transformed_dir_light), dx::XMVectorSet(3.0f, 3.0f, 3.0f, 1.0f));
			//TODO: rename these matrices; it isnt clear that they are used for shadow mapping
			dx::XMMATRIX dir_light_view_matrix = dx::XMMatrixLookAtLH(dir_light_pos, dx::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
			dx::XMMATRIX dir_light_proj_matrix = dx::XMMatrixOrthographicLH(8.0f, 8.0f, dir_light.far_plane_dist, dir_light.near_plane_dist);

			dir_shadow_cb.dir_light_shadow_proj_mat = dx::XMMatrixTranspose(dir_light_proj_matrix);
			dir_shadow_cb.dir_light_shadow_view_mat = dx::XMMatrixTranspose(dir_light_view_matrix);
			omni_dir_shad_cb.dir_light_shadow_proj_mat = dir_shadow_cb.dir_light_shadow_proj_mat;
			omni_dir_shad_cb.dir_light_shadow_view_mat = dir_shadow_cb.dir_light_shadow_view_mat;
		}

		for (int point_light_idx = 0; i < view_desc.num_dir_lights + view_desc.num_point_lights; i++, point_light_idx++)
		{
			const Point_Light& p_light = view_desc.lights_info[i].point_light;
			scene_lights.point_lights[point_light_idx] = p_light;

			dx::XMVECTOR point_light_final_pos = dx::XMVector4Transform(dx::XMLoadFloat4(&p_light.pos_world_space), view_desc.lights_info[i].final_world_transform);
			scene_lights.point_pos_view_space[point_light_idx] = dx::XMVector4Transform(point_light_final_pos, view_desc.cam.get_view_matrix());

			if (view_desc.num_point_lights > 0) omni_dir_shad_cb.point_light_proj_mat = dx::XMMatrixTranspose(dx::XMMatrixPerspectiveFovLH(dx::XM_PIDIV2, 1.0f, view_desc.lights_info[i].point_light.far_plane_dist, view_desc.lights_info[i].point_light.near_plane_dist));

			omni_dir_shad_cb.point_light_view_mat[point_light_idx * 6 + Cubemap::FACE_POSITIVE_X] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(point_light_final_pos, dx::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
			omni_dir_shad_cb.point_light_view_mat[point_light_idx * 6 + Cubemap::FACE_NEGATIVE_X] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(point_light_final_pos, dx::XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
			omni_dir_shad_cb.point_light_view_mat[point_light_idx * 6 + Cubemap::FACE_POSITIVE_Y] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(point_light_final_pos, dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f)));
			omni_dir_shad_cb.point_light_view_mat[point_light_idx * 6 + Cubemap::FACE_NEGATIVE_Y] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(point_light_final_pos, dx::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f)));
			omni_dir_shad_cb.point_light_view_mat[point_light_idx * 6 + Cubemap::FACE_POSITIVE_Z] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(point_light_final_pos, dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
			omni_dir_shad_cb.point_light_view_mat[point_light_idx * 6 + Cubemap::FACE_NEGATIVE_Z] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(point_light_final_pos, dx::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));

			//for (int point_light_idx = 0; i < view_desc.num_dir_lights + view_desc.num_point_lights; i++, point_light_idx++)
			//{
			//	const Point_Light& p_light = view_desc.lights_info[i].point_light;
			//	tmp_lights_cb.point_lights[point_light_idx] = p_light;

			//	dx::XMVECTOR point_light_final_pos = dx::XMVector4Transform(dx::XMLoadFloat4(&p_light.pos_world_space), view_desc.lights_info[i].final_world_transform);
			//	tmp_lights_cb.point_pos_view_space[point_light_idx] = dx::XMVector4Transform(point_light_final_pos, view_desc.cam.get_view_matrix());
			//}
			//tmp_lights_cb.num_point_lights = view_desc.num_point_lights;

		}
		scene_lights.num_point_lights = view_desc.num_point_lights;

		for (int spot_light_idx = 0; i < view_desc.num_dir_lights + view_desc.num_point_lights + view_desc.num_spot_lights; i++, spot_light_idx++)
		{
			const Spot_Light& spot_light = view_desc.lights_info[i].spot_light;
			scene_lights.spot_lights[spot_light_idx] = spot_light;

			dx::XMVECTOR spot_light_final_pos = dx::XMVector4Transform(dx::XMLoadFloat4(&spot_light.pos_world_space), view_desc.lights_info[i].final_world_transform);
			dx::XMVECTOR spot_light_final_dir = dx::XMVector4Transform(dx::XMLoadFloat4(&spot_light.direction), view_desc.lights_info[i].final_world_transform);

			dx::XMStoreFloat4(&scene_lights.spot_lights[spot_light_idx].pos_world_space, spot_light_final_pos);
			dx::XMStoreFloat4(&scene_lights.spot_lights[spot_light_idx].direction, spot_light_final_dir);

			scene_lights.spot_pos_view_space[spot_light_idx] = dx::XMVector4Transform(spot_light_final_pos, view_desc.cam.get_view_matrix());
			scene_lights.spot_dir_view_space[spot_light_idx] = dx::XMVector4Transform(spot_light_final_dir, view_desc.cam.get_view_matrix());

			omni_dir_shad_cb.spot_light_proj_mat[spot_light_idx] = dx::XMMatrixTranspose(dx::XMMatrixPerspectiveFovLH(std::acos(view_desc.lights_info[i].spot_light.cos_cutoff_angle) * 2.0f, 1.0f, view_desc.lights_info[i].spot_light.far_plane_dist, view_desc.lights_info[i].spot_light.near_plane_dist));  //multiply acos by 2, because cutoff angle is considered from center, not entire light angle
			omni_dir_shad_cb.spot_light_view_mat[spot_light_idx] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(spot_light_final_pos, spot_light_final_dir, dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));

		}
		scene_lights.num_spot_ligthts = view_desc.num_spot_lights;
	}
}