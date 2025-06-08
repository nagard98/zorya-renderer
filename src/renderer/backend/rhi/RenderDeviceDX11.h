#pragma once

#include "RenderDevice.h"
#include "RenderDeviceDX11.h"

#include <vector>
#include <unordered_map>

namespace zorya
{

	class DX11_Render_Device : Render_Device<DX11_Render_Device>
	{

	public:
		DX11_Render_Device();
		~DX11_Render_Device();

		void init();

		Result_Code create_texture_2d(Render_Texture_Handle* tex_handle, const D3D11_SUBRESOURCE_DATA* init_data, const Texture_2D_Desc& desc);
		Result_Code create_srv_tex_2d(Render_SRV_Handle* srv_handle, Render_Texture_Handle tex_handle, Texture_Format format, int mip_levels = 1, int most_detailed_mip = 0);
		Result_Code create_rtv_tex_2d(Render_RTV_Handle* rtv_handle, Render_Texture_Handle tex_handle, Texture_Format format, int mip_slice = 0);
		Result_Code create_dsv_tex_2d(Render_DSV_Handle* dsv_handle, Render_Texture_Handle tex_handle, Texture_Format format, int mip_slice = 0, bool is_read_only = false);

		Result_Code create_srv_tex_2d_array(Render_SRV_Handle* srv_handle, Render_Texture_Handle tex_handle, Texture_Format format, int array_size = 1, int first_array_slice = 0, int mipLevels = 1, int most_detailed_mip = 0);
		Result_Code create_dsv_tex_2d_array(Render_DSV_Handle* dsv_handle, Render_Texture_Handle tex_handle, Texture_Format format, int array_size = 1, int mip_slice = 0, int first_array_slice = 0, bool is_read_only = false);
		Result_Code create_rtv_tex_2d_array(Render_RTV_Handle* rtv_handle, Render_Texture_Handle tex_handle, Texture_Format format, int array_size = 1, int mip_slice = 0, int first_array_slice = 0);

		Result_Code create_tex_cubemap(Render_Texture_Handle* tex_handle, Resource_Bind_Flags bind_flags, Texture_Format format, float width, float height, int array_size = 1, Render_SRV_Handle* srv_handle = nullptr, Render_RTV_Handle* rtv_handle = nullptr, bool generate_mips = false, int mip_levels = 1, int sample_count = 1, int sample_quality = 0);
		Result_Code create_srv_tex_cubemap(Render_SRV_Handle* srv_handle, Render_Texture_Handle tex_handle, Texture_Format format, int array_size = 1, int first_array_slice = 0, int mipLevels = 1, int most_detailed_mip = 0);

		Result_Code create_constant_buffer(Constant_Buffer_Handle* hnd, const Buffer_Desc* buffer_desc);

		Result_Code create_pso(PSO_Handle* pso_hnd, const PSO_Desc& pso_desc);
		Result_Code create_pixel_shader(Pixel_Shader_Handle* ps_hnd, const Shader_Bytecode& bytecode);
		Result_Code create_vertex_shader(Vertex_Shader_Handle* vs_hnd, const Shader_Bytecode& bytecode);
		Result_Code create_ds_state(DS_State_Handle* ds_state_hnd, const Depth_Stencil_State_Desc& ds_state_desc);
		Result_Code create_rs_state(RS_State_Handle* rs_state_hnd, const Rasterizer_State_Desc& rs_state_desc);
		Result_Code create_bl_state(BL_State_Handle* bl_state_hnd, const D3D11_BLEND_DESC& bl_state_desc);

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
		ID3D11BlendState* get_bl_state_pointer(const BL_State_Handle bl_hnd) const;
		const Pipeline_State_Object* get_pso_pointer(const PSO_Handle pso_hnd) const;

		DS_State_Handle ds_state_hnd_from_desc(const Depth_Stencil_State_Desc& ds_state_desc);
		RS_State_Handle rs_state_hnd_from_desc(const Rasterizer_State_Desc& rs_state_desc);
		BL_State_Handle bl_state_hnd_from_desc(const D3D11_BLEND_DESC& bl_state_desc);


		void release_all_resources();

		//TODO: move to private when you add all the functionality in this abstraction layer
		ID3D11Device* m_device;

		//TODO: move to private; just temporary public before implementing constant buffer binding API

	private:

		template<UINT TNameLength>
		inline void set_debug_object_name(ID3D11DeviceChild* resource, const char(&name)[TNameLength])
		{
			resource->SetPrivateData(WKPDID_D3DDebugObjectName, TNameLength - 1, name);
		}

		std::vector<Constant_Buffer> m_cb_resources;
		std::vector<ID3D11Texture2D*> m_tex_2d_resources;
		std::vector<ID3D11RenderTargetView*> m_rtv_resources;
		std::vector<ID3D11ShaderResourceView*> m_srv_resources;
		std::vector<ID3D11DepthStencilView*> m_dsv_resources;
		std::unordered_map<uint64_t, PSO_Handle> m_pso_handles;
		std::vector<Pipeline_State_Object> m_pso_resources;
		std::vector<ID3D11PixelShader*> m_ps_resources;
		std::vector<ID3D11VertexShader*> m_vs_resources;
		std::unordered_map<uint64_t, DS_State_Handle> m_ds_state_handles;
		std::vector<ID3D11DepthStencilState*> m_ds_state_resources;
		std::unordered_map<uint64_t, RS_State_Handle> m_rs_state_handles;
		std::unordered_map<uint64_t, BL_State_Handle> m_bl_state_handles;
		std::vector<ID3D11RasterizerState*> m_rs_state_resources;
		std::vector<ID3D11BlendState*> m_bl_state_resources;
		std::vector<ID3D11InputLayout*> m_input_layout_resources;

		int m_tex_2d_count{ 1 }, m_rtv_count{ 1 }, m_srv_count{ 1 }, m_dsv_count{ 1 }, m_const_buff_count{ 1 }, m_pso_count{ 1 }, m_ps_count{ 1 }, m_vs_count{ 1 }, m_ds_state_count{ 1 }, m_rs_state_count{ 1 }, m_bl_state_count{ 1 }, m_input_layout_count{ 1 };

	};


}