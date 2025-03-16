#ifndef RENDER_DEVICE_H_
#define RENDER_DEVICE_H_

#include <iostream>
#include <cstdint>
#include <d3d11_1.h>
#include <wrl/client.h>
#include <vector>
#include <unordered_map>

#include "Platform.h"

#include <renderer/backend/ConstantBuffer.h>
#include <renderer/backend/PipelineStateObject.h>
#include <renderer/backend/rhi/RenderDeviceHandles.h>

namespace zorya
{
	namespace wrl = Microsoft::WRL;

	//TODO: correct types for all these structs
	struct ZRY_Result
	{
		HRESULT value;
	};

	struct ZRY_Format
	{
		DXGI_FORMAT value;
	};

	struct ZRY_Bind_Flags
	{
		UINT value;
	};

	struct ZRY_Usage
	{
		D3D11_USAGE value;
	};

	const D3D11_DEPTH_STENCIL_DESC default_ds_desc{ 
		true, 
		D3D11_DEPTH_WRITE_MASK_ALL, 
		D3D11_COMPARISON_LESS, 
		false, 
		D3D11_DEFAULT_STENCIL_READ_MASK, 
		D3D11_DEFAULT_STENCIL_WRITE_MASK, 
		D3D11_DEPTH_STENCILOP_DESC{D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS}, 
		D3D11_DEPTH_STENCILOP_DESC{D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS} 
	};
	
	const D3D11_RASTERIZER_DESC default_rs_desc{ 
		D3D11_FILL_SOLID,
		D3D11_CULL_BACK,
		false,
		0,
		0.0f,
		0.0f,
		true,
		false,
		false,
		false
	};

	const D3D11_BLEND_DESC default_bl_desc{
		false,
		false,
		{ D3D11_RENDER_TARGET_BLEND_DESC{false} }
	};

	constexpr D3D11_BLEND_DESC create_default_blend_desc()
	{
		D3D11_BLEND_DESC desc{};
		desc.AlphaToCoverageEnable = false;
		desc.IndependentBlendEnable = false;
		
		desc.RenderTarget[0].BlendEnable = false;
		desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
		desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		desc.RenderTarget[1].BlendEnable = false;
		desc.RenderTarget[2].BlendEnable = false;
		desc.RenderTarget[3].BlendEnable = false;
		desc.RenderTarget[4].BlendEnable = false;
		desc.RenderTarget[5].BlendEnable = false;
		desc.RenderTarget[6].BlendEnable = false;
		desc.RenderTarget[7].BlendEnable = false;

		return desc;	
	}

	PSO_Desc create_default_gbuff_desc();
	//static PSO_Desc create_default_gbuff_desc()
	//{
	//	PSO_Desc pso_desc{};
	//	pso_desc.pixel_shader_bytecode = Pixel_Shader::s_pixel_shader_bytecode_buffers[(uint8_t)PShader_ID::STANDARD];
	//	pso_desc.vertex_shader_bytecode = Vertex_Shader::s_vertex_shader_bytecode_buffers[(uint8_t)VShader_ID::STANDARD];

	//	pso_desc.rasterizer_desc.CullMode = D3D11_CULL_BACK;
	//	pso_desc.rasterizer_desc.FillMode = D3D11_FILL_SOLID;
	//	pso_desc.rasterizer_desc.FrontCounterClockwise = false;
	//	pso_desc.rasterizer_desc.DepthClipEnable = true;

	//	pso_desc.depth_stencil_desc.DepthEnable = true;
	//	pso_desc.depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	//	pso_desc.depth_stencil_desc.DepthFunc = D3D11_COMPARISON_GREATER;

	//	pso_desc.depth_stencil_desc.StencilEnable = true;
	//	pso_desc.depth_stencil_desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	//	pso_desc.depth_stencil_desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	//	pso_desc.depth_stencil_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	//	pso_desc.depth_stencil_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	//	pso_desc.depth_stencil_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	//	pso_desc.depth_stencil_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;

	//	pso_desc.depth_stencil_desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	//	pso_desc.depth_stencil_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	//	pso_desc.depth_stencil_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	//	pso_desc.depth_stencil_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;

	//	pso_desc.stencil_ref_value = 1;

	//	pso_desc.input_elements_desc = s_vertex_layout_desc;
	//	pso_desc.num_elements = sizeof(s_vertex_layout_desc) / sizeof(s_vertex_layout_desc[0]);

	//	pso_desc.blend_desc = create_default_blend_desc();

	//	return pso_desc;
	//}

	template <class T>
	class Render_Device
	{

	public:
		ZRY_Result create_tex_2d()
		{
			return impl().create_tex_2d();
		}

	private:
		T& impl()
		{
			return *static_cast<T*>(this);
		}

	};


	class DX11_Render_Device : Render_Device<DX11_Render_Device>
	{

	public:
		DX11_Render_Device();
		~DX11_Render_Device();

		void init();

		//template <typename T>
		//ZRY_Result create_constant_buffer(constant_buffer_handle<T>* hnd_constant_buffer, const char* name)
		//{
		//	D3D11_BUFFER_DESC buffer_desc{};
		//	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		//	buffer_desc.ByteWidth = sizeof(T);

		//	Constant_Buffer& const_buffer = m_cb_resources.at(m_const_buff_count);
		//	const_buffer.constant_buffer_name = name;

		//	HRESULT hr = m_device->CreateBuffer(&buffer_desc, nullptr, &const_buffer.buffer);
		//	if (!FAILED(hr))
		//	{
		//		hnd_constant_buffer->index = m_const_buff_count;
		//		m_const_buff_count += 1;
		//	}

		//	return ZRY_Result{ hr };
		//}


		ZRY_Result create_tex_2d(Render_Texture_Handle* tex_handle, const D3D11_SUBRESOURCE_DATA* init_data, ZRY_Usage usage, ZRY_Bind_Flags bind_flags, ZRY_Format format, float width, float height, int array_size = 1, Render_SRV_Handle* srv_handle = nullptr, Render_RTV_Handle* rtv_handle = nullptr, bool generate_mips = false, int mip_levels = 1, int sample_count = 1, int sample_quality = 0);
		ZRY_Result create_srv_tex_2d(Render_SRV_Handle* srv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int mip_levels = 1, int most_detailed_mip = 0);
		ZRY_Result create_rtv_tex_2d(Render_RTV_Handle* rtv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int mip_slice = 0);
		ZRY_Result create_dsv_tex_2d(Render_DSV_Handle* dsv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int mip_slice = 0, bool is_read_only = false);

		ZRY_Result create_srv_tex_2d_array(Render_SRV_Handle* srv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int array_size = 1, int first_array_slice = 0, int mipLevels = 1, int most_detailed_mip = 0);
		ZRY_Result create_dsv_tex_2d_array(Render_DSV_Handle* dsv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int array_size = 1, int mip_slice = 0, int first_array_slice = 0, bool is_read_only = false);
		ZRY_Result create_rtv_tex_2d_array(Render_RTV_Handle* rtv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int array_size = 1, int mip_slice = 0, int first_array_slice = 0);

		ZRY_Result create_tex_cubemap(Render_Texture_Handle* tex_handle, ZRY_Bind_Flags bind_flags, ZRY_Format format, float width, float height, int array_size = 1, Render_SRV_Handle* srv_handle = nullptr, Render_RTV_Handle* rtv_handle = nullptr, bool generate_mips = false, int mip_levels = 1, int sample_count = 1, int sample_quality = 0);
		ZRY_Result create_srv_tex_cubemap(Render_SRV_Handle* srv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int array_size = 1, int first_array_slice = 0, int mipLevels = 1, int most_detailed_mip = 0);

		ZRY_Result create_constant_buffer(Constant_Buffer_Handle* hnd, const D3D11_BUFFER_DESC* buffer_desc);

		ZRY_Result create_pso(PSO_Handle* pso_hnd, const PSO_Desc& pso_desc);
		ZRY_Result create_pixel_shader(Pixel_Shader_Handle* ps_hnd, const Shader_Bytecode& bytecode);
		ZRY_Result create_vertex_shader(Vertex_Shader_Handle* vs_hnd, const Shader_Bytecode& bytecode);
		ZRY_Result create_ds_state(DS_State_Handle* ds_state_hnd, const D3D11_DEPTH_STENCIL_DESC& ds_state_desc);
		ZRY_Result create_rs_state(RS_State_Handle* rs_state_hnd, const D3D11_RASTERIZER_DESC& rs_state_desc);
		ZRY_Result create_bl_state(BL_State_Handle* bl_state_hnd, const D3D11_BLEND_DESC& bl_state_desc);

		Render_SRV_Handle add_srv(ID3D11ShaderResourceView*&& srv_resource);

		//template <typename T>
		//Constant_Buffer* get_cb_pointer(const constant_buffer_handle<T> cb_hnd)
		//{
		//	zassert(cb_hnd.index < m_const_buff_count);
		//	return &m_cb_resources.at(cb_hnd.index);
		//}

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

		DS_State_Handle ds_state_hnd_from_desc(const D3D11_DEPTH_STENCIL_DESC&  ds_state_desc);
		RS_State_Handle rs_state_hnd_from_desc(const D3D11_RASTERIZER_DESC&  rs_state_desc);
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

		int m_tex_2d_count{1}, m_rtv_count{1}, m_srv_count{1}, m_dsv_count{1}, m_const_buff_count{1}, m_pso_count{1}, m_ps_count{1},
			m_vs_count{ 1 }, m_ds_state_count{ 1 }, m_rs_state_count{ 1 }, m_bl_state_count{ 1 }, m_input_layout_count{ 1 };

	};



}
#endif