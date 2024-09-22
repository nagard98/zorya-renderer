#ifndef RENDER_HARDWARE_INTERFACE_H_
#define RENDER_HARDWARE_INTERFACE_H_

#include <d3d11_1.h>
#include <wrl/client.h>
#include <cstdint>
#include <array>

#include "Platform.h"

#include "renderer/backend/rhi/RHIState.h"
#include "renderer/backend/rhi/RenderDevice.h"
#include "renderer/backend/rhi/RenderDeviceHandles.h"
#include "renderer/backend/GraphicsContext.h"
#include "renderer/backend/PipelineStateObject.h"
#include "renderer/backend/RenderGraph.h"

#include "renderer/frontend/Material.h"
#include "renderer/frontend/Shader.h"
#include "renderer/frontend/Asset.h"
#include "renderer/frontend/Texture2D.h"

#define MULTISAMPLE_COUNT 1

#define RHI_OK uint8_t(0)
#define RHI_ERR uint8_t(1)

namespace zorya
{
	namespace wrl = Microsoft::WRL;

	typedef uint8_t RHI_RESULT;

	template <typename T, typename I>
	struct State_Variant_Ptr
	{
		RHI_State variant = -1;
		T* p_state_object;
	};

	class Render_Hardware_Interface
	{

	public:
		Render_Hardware_Interface();
		~Render_Hardware_Interface();

		HRESULT init(HWND window_handle, RHI_State initial_state = RHI_DEFAULT_STATE());
		void shutdown();

		RHI_RESULT load_texture(const wchar_t* path, Shader_Texture2D** shader_texture, bool is_srgb, size_t max_size = 0);
		RHI_RESULT load_texture2(const Texture2D* const texture_asset, const Texture_Import_Config* const tex_config, Render_SRV_Handle* hnd_srv, size_t max_size = 0);

		ZRY_Result create_tex(Render_Texture_Handle* tex_handle, const D3D11_SUBRESOURCE_DATA* init_data, const Render_Graph_Resource_Metadata& meta);
		ZRY_Result create_srv(Render_SRV_Handle* srv_handle, Render_Texture_Handle* tex_handle, const Render_Graph_Resource_Metadata& meta, const Render_Graph_View_Desc& view_desc);
		ZRY_Result create_dsv(Render_DSV_Handle* dsv_handle, Render_Texture_Handle* tex_handle, const Render_Graph_Resource_Metadata& meta, const Render_Graph_View_Desc& view_desc);
		ZRY_Result create_rtv(Render_RTV_Handle* rtv_handle, Render_Texture_Handle* tex_handle, const Render_Graph_Resource_Metadata& meta, const Render_Graph_View_Desc& view_desc);

		ZRY_Result create_tex_2d(Render_Texture_Handle* tex_handle, const D3D11_SUBRESOURCE_DATA* init_data, ZRY_Usage usage, ZRY_Bind_Flags bind_flags, ZRY_Format format, float width, float height, int array_size=1, Render_SRV_Handle* srv_handle = nullptr, Render_RTV_Handle* rtv_handle = nullptr, bool generate_mips = false, int mip_levels = 0, int sample_count = 1, int sample_quality = 0);
		ZRY_Result create_srv_tex_2d(Render_SRV_Handle* srv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int mip_levels = 1, int most_detailed_mip = 0);
		ZRY_Result create_rtv_tex_2d(Render_RTV_Handle* rtv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int mip_slice = 0);
		ZRY_Result create_dsv_tex_2d(Render_DSV_Handle* dsv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int mip_slice = 0, bool is_read_only = false);

		ZRY_Result create_tex_cubemap(Render_Texture_Handle* tex_handle, ZRY_Bind_Flags bind_flags, ZRY_Format format, float width, float height, int array_size = 1, Render_SRV_Handle* srv_handle = nullptr, Render_RTV_Handle* rtv_handle = nullptr, bool generate_mips = false, int mip_levels = 0,  int sample_count = 1, int sample_quality = 0);
		ZRY_Result create_srv_tex_2d_array(Render_SRV_Handle* srv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int array_size = 1, int first_array_slice = 0, int mipLevels = 1, int most_detailed_mip = 0);
		ZRY_Result create_dsv_tex_2d_array(Render_DSV_Handle* dsv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int array_size = 1, int mip_slice = 0, int first_array_slice = 0);

		ZRY_Result create_constant_buffer(Constant_Buffer_Handle* hnd, const D3D11_BUFFER_DESC* buffer_desc);

		ZRY_Result create_pso(PSO_Handle* pso_hnd, const PSO_Desc& pso_desc);
		ZRY_Result create_pixel_shader(Pixel_Shader_Handle* ps_hnd, const Shader_Bytecode& bytecode);
		ZRY_Result create_vertex_shader(Vertex_Shader_Handle* vs_hnd, const Shader_Bytecode& bytecode);
		ZRY_Result create_ds_state(DS_State_Handle* ds_state_hnd, const D3D11_DEPTH_STENCIL_DESC& ds_state_desc);
		ZRY_Result create_rs_state(RS_State_Handle* rs_state_hnd, const D3D11_RASTERIZER_DESC& rs_state_desc);

		Render_SRV_Handle add_srv(ID3D11ShaderResourceView*&& srv_resource);

		//TODO: remove this method when rest of abstraction is implemented?
		ID3D11Texture2D* get_tex_2d_pointer(const Render_Texture_Handle rt_hnd) const;
		ID3D11RenderTargetView* get_rtv_pointer(const Render_RTV_Handle rtv_hnd) const;
		ID3D11ShaderResourceView* get_srv_pointer(const Render_SRV_Handle srv_hnd) const;
		ID3D11DepthStencilView* get_dsv_pointer(const Render_DSV_Handle dsv_hnd) const;
		ID3D11Buffer* get_cb_pointer(const Constant_Buffer_Handle cb_hnd) const;
		ID3D11PixelShader* get_ps_pointer(const Pixel_Shader_Handle ps_hnd) const;
		ID3D11VertexShader* get_vs_pointer(const Vertex_Shader_Handle vs_hnd) const;
		ID3D11DepthStencilState* get_ds_state_pointer(const DS_State_Handle ds_hnd) const;
		ID3D11RasterizerState* get_rs_state_pointer(const RS_State_Handle rs_hnd) const;
		const Pipeline_State_Object* get_pso_pointer(const PSO_Handle pso_hnd) const;

		DS_State_Handle ds_state_hnd_from_desc(const D3D11_DEPTH_STENCIL_DESC& ds_state_desc);
		RS_State_Handle rs_state_hnd_from_desc(const D3D11_RASTERIZER_DESC& rs_state_desc);

		HRESULT resize_window(uint32_t width, uint32_t height);

		void release_all_resources();		

		ID3D11DeviceContext* m_context;
		GraphicsContext* m_gfx_context;

		ID3D11Debug* m_debug_layer;

		IDXGISwapChain* m_swap_chain;

		ID3D11RenderTargetView* m_back_buffer_rtv;

		ID3D11Texture2D* m_back_buffer_depth_tex;
		ID3D11DepthStencilView* m_back_buffer_depth_dsv;
		ID3D11ShaderResourceView* m_back_buffer_depth_srv;

		D3D_FEATURE_LEVEL m_feature_level; //feature level found for device
		D3D11_VIEWPORT m_viewport;
		DX11_Render_Device m_device;

		Root_Signature gfx_root_signature;

	private:
		D3D11_RASTERIZER_DESC m_tmp_rast_desc;
		D3D11_DEPTH_STENCIL_DESC m_tmp_depth_sten_desc;

	};

	extern Render_Hardware_Interface rhi;

}
#endif