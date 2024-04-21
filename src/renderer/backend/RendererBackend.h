#ifndef RENDERER_BACKEND_H_
#define RENDERER_BACKEND_H_

#include "renderer/backend/BufferCache.h"
#include "renderer/backend/rhi/RenderDevice.h"

#include "renderer/frontend/RendererFrontend.h"
#include "renderer/frontend/Lights.h"
#include "renderer/frontend/Shader.h"

#include <functional>
#include <cstdint>
#include <d3d11_1.h>

namespace zorya
{
	struct Cubemap
	{
		enum : int
		{
			FACE_POSITIVE_X = 0,
			FACE_NEGATIVE_X = 1,
			FACE_POSITIVE_Y = 2,
			FACE_NEGATIVE_Y = 3,
			FACE_POSITIVE_Z = 4,
			FACE_NEGATIVE_Z = 5,
		};
	};

	struct G_Buffer
	{
		enum :int
		{
			ALBEDO,
			NORMAL,
			ROUGH_MET,
			SIZE
		};
	};

	enum Constan_Buffer_Type
	{
		CB_Application,
		CB_Frame,
		CB_Object,
		CB_Light,
		NumConstantBuffers
	};

	struct Obj_CB
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


	class Renderer_Backend
	{

	public:
		Renderer_Backend();
		~Renderer_Backend();

		//TODO: change return type to custom wrapper
		HRESULT init(bool reset = false);
		void render_view(const View_Desc& view_desc);

		void release_all_resources();

		ID3DUserDefinedAnnotation* m_annot;
		D3D11_VIEWPORT m_scene_viewport;

		ID3D11Texture2D* m_final_render_target_tex;
		ID3D11RenderTargetView* m_final_render_target_view;
		ID3D11ShaderResourceView* m_final_render_target_srv;

		ID3D11Texture2D* m_depth_tex;
		ID3D11DepthStencilView* m_depth_dsv;
		ID3D11ShaderResourceView* m_depth_srv;

	private:
		void render_shadow_maps(const View_Desc& view_desc, Dir_Shadow_CB& dir_shadow_cb, Omni_Dir_Shadow_CB& cb_omni_dir_shad);

		struct Application_Constant_Buff
		{

		};

		struct Frame_Constant_Buff
		{
			Scene_Lights m_scene_lights;
			Subsurface_Scattering_Params sss_params;
		};

		struct Object_Constant_Buff
		{
			Material_Params material_params;
		};

		constant_buffer_handle<Frame_Constant_Buff> m_hnd_frame_cb;
		constant_buffer_handle<Object_Constant_Buff> m_hnd_object_cb;

		//ID3D11Buffer* matPrmsCB;

		ID3D11Buffer* m_object_cb;
		ID3D11Buffer* m_view_cb;
		ID3D11Buffer* m_proj_cb;
		ID3D11Buffer* m_dir_shad_cb;
		ID3D11Buffer* m_omni_dir_shad_cb;

		ID3D11Buffer* m_inv_mat_cb;

		D3D11_VIEWPORT m_shadow_map_viewport;

		//TODO:Thickness map here is temporary; move to material
		Shader_Texture2D m_thickness_map_srv;

		ID3D11Texture2D* m_gbuffer[G_Buffer::SIZE];
		ID3D11RenderTargetView* m_gbuffer_rtv[G_Buffer::SIZE];
		ID3D11ShaderResourceView* m_gbuffer_srv[G_Buffer::SIZE];

		ID3D11Texture2D* m_ambient_map;
		ID3D11RenderTargetView* m_ambient_rtv;
		ID3D11ShaderResourceView* m_ambient_srv;

		ID3D11Texture2D* m_shadow_map;
		ID3D11ShaderResourceView* m_shadow_map_srv;
		ID3D11DepthStencilView* m_shadow_map_dsv;

		ID3D11Texture2D* m_spot_shadow_map;
		ID3D11ShaderResourceView* m_spot_shadow_map_srv;
		ID3D11DepthStencilView* m_spot_shadow_map_desv;

		ID3D11Texture2D* m_shadow_cube_map;
		ID3D11ShaderResourceView* m_shadow_cube_map_srv;
		ID3D11DepthStencilView* m_shadow_cube_map_dsv[6 * 2];

		ID3D11Texture2D* m_skin_maps[5];
		ID3D11RenderTargetView* m_skin_rt[5];
		ID3D11ShaderResourceView* m_skin_srv[5];

		ID3D11ShaderResourceView* m_cubemap_view; //skybox view

		Vertex_Shader m_shadow_map_vertex_shader;
		Pixel_Shader m_shadow_map_pixel_shader;

		Vertex_Shader m_gbuffer_vertex_shader;
		Vertex_Shader m_fullscreen_quad_shader;

		Pixel_Shader m_lighting_shader;

		Vertex_Shader m_skybox_vertex_shader;
		Pixel_Shader m_skybox_pixel_shader;

		Pixel_Shader m_sssss_pixel_shader;

		//TODO: what did I intend to do with this?
		//std::hash<std::uint16_t> submeshHash;
	};

	extern Renderer_Backend rb;

}
#endif // !RENDERER_BACKEND_H_

