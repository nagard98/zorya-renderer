#ifndef RENDERER_H_
#define RENDERER_H_

#include "renderer/backend/BufferCache.h"
#include "renderer/backend/rhi/RenderDevice.h"
#include "renderer/backend/GraphicsContext.h"

#include "renderer/frontend/SceneManager.h"
#include "renderer/frontend/Lights.h"
#include "renderer/frontend/Shader.h"
#include "renderer/frontend/DiffusionProfile.h"

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

	struct Cam_Transformation
	{
		dx::XMMATRIX world_matrix;
		dx::XMMATRIX view_matrix;
		dx::XMMATRIX proj_matrix;
	};

	struct Light_Draw_Constants
	{
		uint32_t light_index;
		uint32_t light_type;
		uint32_t face;
		uint32_t pad;
	};

	class Renderer
	{

	public:
		Renderer();
		~Renderer();

		//TODO: change return type to custom wrapper
		HRESULT init(bool reset = false);
		void shutdown();

		void render_view(const View_Desc& view_desc);

		void release_all_resources();

		Render_Rect m_scene_viewport;

		Render_Texture_Handle hnd_final_rt;
		Render_SRV_Handle hnd_final_srv;
		Render_RTV_Handle hnd_final_rtv;

	private:

		struct Application_Constant_Buff
		{

		};

		struct Frame_Constant_Buff
		{
			Scene_Lights m_scene_lights;
			Subsurface_Scattering_Params sss_params[64];
		};

		struct Object_Constant_Buff
		{
			//TODO:fix (this is temporary, until i fix the rest of the architecture)
			Material_Params material_params;
		};

		Constant_Buffer_Handle m_hnd_frame_cb;
		Constant_Buffer_Handle m_hnd_object_cb;

		Constant_Buffer_Handle hnd_cam_transf_cb;
		Constant_Buffer_Handle hnd_light_draw_cb;
		Constant_Buffer_Handle hnd_sss_draw_cb;
		Constant_Buffer_Handle hnd_world_cb;
		Constant_Buffer_Handle hnd_view_cb;
		Constant_Buffer_Handle hnd_proj_cb;
		Constant_Buffer_Handle hnd_dir_shad_cb;
		Constant_Buffer_Handle hnd_omni_dir_shad_cb;

		Constant_Buffer_Handle hnd_inv_mat_cb;

		//D3D11_VIEWPORT m_shadow_map_viewport;
		Render_Rect m_shadow_map_viewport;

		Render_SRV_Handle hnd_sky_cubemap_srv;

		//PSO_Handle hnd_shadow_map_creation_pso;
		//PSO_Handle hnd_shadow_mapping_pso;
		//PSO_Handle hnd_lighting_pso;
		//PSO_Handle hnd_sss_golubev_pso;
		//PSO_Handle hnd_shadow_mask_pso;
		//PSO_Handle hnd_skybox_pso;
		//PSO_Handle hnd_composit_pso; //simple, new version

	};

	extern Renderer renderer;

}
#endif // !RENDERER_BACKEND_H_

