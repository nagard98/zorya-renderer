#include "RenderDevice.h"
#include "xxhash.h"

#include <wrl/client.h>
#include <cstdint>
#include <d3d11_1.h>
#include <vector>
#include <cassert>

namespace zorya
{
	namespace wrl = Microsoft::WRL;

	#define SOFT_LIMIT_RESOURCE_TYPE 256

	DX11_Render_Device::DX11_Render_Device() :
		m_tex_2d_resources(SOFT_LIMIT_RESOURCE_TYPE),
		m_rtv_resources(SOFT_LIMIT_RESOURCE_TYPE),
		m_srv_resources(SOFT_LIMIT_RESOURCE_TYPE),
		m_dsv_resources(SOFT_LIMIT_RESOURCE_TYPE),
		m_cb_resources(SOFT_LIMIT_RESOURCE_TYPE),
		m_pso_resources(SOFT_LIMIT_RESOURCE_TYPE),
		m_ps_resources(SOFT_LIMIT_RESOURCE_TYPE),
		m_vs_resources(SOFT_LIMIT_RESOURCE_TYPE),
		m_ds_state_resources(SOFT_LIMIT_RESOURCE_TYPE),
		m_rs_state_resources(SOFT_LIMIT_RESOURCE_TYPE),
		m_bl_state_resources(SOFT_LIMIT_RESOURCE_TYPE),
		m_input_layout_resources(SOFT_LIMIT_RESOURCE_TYPE),
		m_tex_2d_count(1),
		m_rtv_count(1),
		m_srv_count(1),
		m_dsv_count(1),
		m_const_buff_count(1),
		m_pso_count(1),
		m_vs_count(1),
		m_ps_count(1),
		m_ds_state_count(1),
		m_rs_state_count(1),
		m_bl_state_count(1),
		m_input_layout_count(1)
	{}

	DX11_Render_Device::~DX11_Render_Device()
	{}

	void DX11_Render_Device::init()
	{
		DS_State_Handle ds_state_hnd;
		create_ds_state(&ds_state_hnd, default_ds_desc);
		m_ds_state_handles.insert({XXH64(&default_ds_desc, sizeof(default_ds_desc), 0) , ds_state_hnd});
		
		RS_State_Handle rs_state_hnd;
		create_rs_state(&rs_state_hnd, default_rs_desc);
		m_rs_state_handles.insert({XXH64(&default_rs_desc, sizeof(default_rs_desc), 0) , rs_state_hnd});
	}

	ZRY_Result DX11_Render_Device::create_tex_2d(Render_Texture_Handle* tex_handle, const D3D11_SUBRESOURCE_DATA* init_data, ZRY_Usage usage, ZRY_Bind_Flags bind_flags, ZRY_Format format, float width, float height, int array_size, Render_SRV_Handle* srv_handle, Render_RTV_Handle* rtv_handle, bool generate_mips, int mip_levels, int sample_count, int sample_quality)
	{
		assert(tex_handle != nullptr);

		D3D11_TEXTURE2D_DESC tex_2d_desc;
		ZeroMemory(&tex_2d_desc, sizeof(tex_2d_desc));
		tex_2d_desc.Format = format.value;
		tex_2d_desc.MipLevels = mip_levels;
		tex_2d_desc.ArraySize = array_size;
		tex_2d_desc.BindFlags = bind_flags.value; //(srv_handle != nullptr ? D3D11_BIND_SHADER_RESOURCE : 0) | (rtv_handle != nullptr ? D3D11_BIND_RENDER_TARGET : 0);

		tex_2d_desc.Width = width;
		tex_2d_desc.Height = height;
		tex_2d_desc.SampleDesc.Count = sample_count;
		tex_2d_desc.SampleDesc.Quality = sample_quality;

		//TODO:usage/access_flags/misc_flags
		tex_2d_desc.Usage = usage.value;
		tex_2d_desc.CPUAccessFlags = 0;
		tex_2d_desc.MiscFlags = generate_mips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

		ZRY_Result zr{ S_OK };
		zr.value = m_device->CreateTexture2D(&tex_2d_desc, init_data, &m_tex_2d_resources.at(m_tex_2d_count));
		RETURN_IF_FAILED_ZRY(zr);

		if (srv_handle != nullptr)
		{
			zr.value = m_device->CreateShaderResourceView(m_tex_2d_resources.at(m_tex_2d_count), nullptr, &m_srv_resources.at(m_srv_count));
			RETURN_IF_FAILED_ZRY(zr);
			set_debug_object_name(m_srv_resources.at(m_srv_count), "tex_srv");
			//TODO: better handle creation for resources
			srv_handle->index = m_srv_count;
			m_srv_count += 1;
		}
		if (rtv_handle != nullptr)
		{
			zr.value = m_device->CreateRenderTargetView(m_tex_2d_resources.at(m_tex_2d_count), nullptr, &m_rtv_resources.at(m_rtv_count));
			RETURN_IF_FAILED_ZRY(zr);
			set_debug_object_name(m_rtv_resources.at(m_rtv_count), "tex_rtv");

			//TODO: better handle creation for resources
			rtv_handle->index = m_rtv_count;
			m_rtv_count += 1;
		}

		tex_handle->index = m_tex_2d_count;
		m_tex_2d_count += 1;
		

		return zr;
	}

	ZRY_Result DX11_Render_Device::create_srv_tex_2d(Render_SRV_Handle* srv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int mip_levels, int most_detailed_map)
	{
		assert(srv_handle != nullptr);

		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
		ZeroMemory(&srv_desc, sizeof(srv_desc));

		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Format = format.value;
		srv_desc.Texture2D.MipLevels = mip_levels;
		srv_desc.Texture2D.MostDetailedMip = most_detailed_map;

		ZRY_Result zr{ S_OK };

		zr.value = m_device->CreateShaderResourceView(get_tex_2d_pointer(*tex_handle), &srv_desc, &m_srv_resources.at(m_srv_count));
		RETURN_IF_FAILED_ZRY(zr);

		srv_handle->index = m_srv_count;
		m_srv_count += 1;

		return zr;
	}

	ZRY_Result DX11_Render_Device::create_rtv_tex_2d(Render_RTV_Handle* rtv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int mip_slice)
	{
		assert(rtv_handle != nullptr);

		D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
		ZeroMemory(&rtv_desc, sizeof(rtv_desc));

		rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtv_desc.Format = format.value;
		rtv_desc.Texture2D.MipSlice = mip_slice;

		ZRY_Result zr{ S_OK };
		zr.value = m_device->CreateRenderTargetView(get_tex_2d_pointer(*tex_handle), &rtv_desc, &m_rtv_resources.at(m_rtv_count));
		RETURN_IF_FAILED_ZRY(zr);
		rtv_handle->index = m_rtv_count;
		m_rtv_count += 1;

		return zr;
	}

	ZRY_Result DX11_Render_Device::create_dsv_tex_2d(Render_DSV_Handle* dsv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int mip_slice, bool is_read_only)
	{
		assert(dsv_handle != nullptr);

		D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
		ZeroMemory(&dsv_desc, sizeof(dsv_desc));

		dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsv_desc.Format = format.value;
		dsv_desc.Texture2D.MipSlice = mip_slice;
		dsv_desc.Flags = is_read_only ? D3D11_DSV_READ_ONLY_DEPTH : 0;

		ZRY_Result zr{ S_OK };
		zr.value = m_device->CreateDepthStencilView(get_tex_2d_pointer(*tex_handle), &dsv_desc, &m_dsv_resources.at(m_dsv_count));
		RETURN_IF_FAILED_ZRY(zr);
		dsv_handle->index = m_dsv_count;
		m_dsv_count += 1;


		return zr;
	}

	ZRY_Result DX11_Render_Device::create_tex_cubemap(Render_Texture_Handle* tex_handle, ZRY_Bind_Flags bind_flags, ZRY_Format format, float width, float height, int array_size, Render_SRV_Handle* srv_handle, Render_RTV_Handle* rtv_handle, bool generate_mips, int mip_levels, int sample_count, int sample_quality)
	{
		assert(tex_handle != nullptr);

		D3D11_TEXTURE2D_DESC tex_2d_desc;
		ZeroMemory(&tex_2d_desc, sizeof(tex_2d_desc));
		tex_2d_desc.Format = format.value;
		tex_2d_desc.MipLevels = mip_levels;
		tex_2d_desc.ArraySize = array_size;
		tex_2d_desc.BindFlags = bind_flags.value; //(srv_handle != nullptr ? D3D11_BIND_SHADER_RESOURCE : 0) | (rtv_handle != nullptr ? D3D11_BIND_RENDER_TARGET : 0);

		tex_2d_desc.Width = width;
		tex_2d_desc.Height = height;
		tex_2d_desc.SampleDesc.Count = sample_count;
		tex_2d_desc.SampleDesc.Quality = sample_quality;

		//TODO:usage/access_flags/misc_flags
		tex_2d_desc.Usage = D3D11_USAGE_DEFAULT;
		tex_2d_desc.CPUAccessFlags = 0;
		tex_2d_desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | (generate_mips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0);

		ZRY_Result zr{ S_OK };

		zr.value = m_device->CreateTexture2D(&tex_2d_desc, nullptr, &m_tex_2d_resources.at(m_tex_2d_count));
		RETURN_IF_FAILED_ZRY(zr);
		//TODO: better handle creation for resources

		if (srv_handle != nullptr)
		{
			zr.value = m_device->CreateShaderResourceView(m_tex_2d_resources.at(m_tex_2d_count), nullptr, &m_srv_resources.at(m_srv_count));
			RETURN_IF_FAILED_ZRY(zr);
			//TODO: better handle creation for resources
			srv_handle->index = m_srv_count;
			m_srv_count += 1;
		}
		if (rtv_handle != nullptr)
		{
			zr.value = m_device->CreateRenderTargetView(m_tex_2d_resources.at(m_tex_2d_count), nullptr, &m_rtv_resources.at(m_rtv_count));
			RETURN_IF_FAILED_ZRY(zr);
			//TODO: better handle creation for resources
			rtv_handle->index = m_rtv_count;
			m_rtv_count += 1;
		}

		tex_handle->index = m_tex_2d_count;
		m_tex_2d_count += 1;
		

		return zr;
	}

	ZRY_Result DX11_Render_Device::create_srv_tex_2d_array(Render_SRV_Handle* srv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int array_size, int first_array_slice, int mip_levels, int most_detailed_map)
	{
		assert(srv_handle);

		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
		ZeroMemory(&srv_desc, sizeof(srv_desc));

		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		srv_desc.Format = format.value;
		srv_desc.Texture2DArray.MipLevels = mip_levels;
		srv_desc.Texture2DArray.MostDetailedMip = most_detailed_map;
		srv_desc.Texture2DArray.ArraySize = array_size;
		srv_desc.Texture2DArray.FirstArraySlice = first_array_slice;

		ZRY_Result zr{ S_OK };

		zr.value = m_device->CreateShaderResourceView(get_tex_2d_pointer(*tex_handle), &srv_desc, &m_srv_resources.at(m_srv_count));
		RETURN_IF_FAILED_ZRY(zr);
		srv_handle->index = m_srv_count;
		m_srv_count += 1;
		
		return zr;
	}

	ZRY_Result DX11_Render_Device::create_srv_tex_cubemap(Render_SRV_Handle* srv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int array_size, int first_array_slice, int mipLevels, int most_detailed_mip)
	{
		assert(srv_handle);

		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
		ZeroMemory(&srv_desc, sizeof(srv_desc));

		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		srv_desc.Format = format.value;
		srv_desc.Texture2DArray.MipLevels = mipLevels;
		srv_desc.Texture2DArray.MostDetailedMip = most_detailed_mip;
		srv_desc.Texture2DArray.ArraySize = array_size;
		srv_desc.Texture2DArray.FirstArraySlice = first_array_slice;

		ZRY_Result zr{ S_OK };

		zr.value = m_device->CreateShaderResourceView(get_tex_2d_pointer(*tex_handle), &srv_desc, &m_srv_resources.at(m_srv_count));
		RETURN_IF_FAILED_ZRY(zr);
		srv_handle->index = m_srv_count;
		m_srv_count += 1;

		return zr;
	}


	ZRY_Result DX11_Render_Device::create_dsv_tex_2d_array(Render_DSV_Handle* dsv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int array_size, int mip_slice, int first_array_slice, bool is_read_only)
	{
		assert(dsv_handle != nullptr);

		D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
		ZeroMemory(&dsv_desc, sizeof(dsv_desc));

		dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		dsv_desc.Format = format.value;
		dsv_desc.Texture2DArray.ArraySize = array_size;
		dsv_desc.Texture2DArray.FirstArraySlice = first_array_slice;
		dsv_desc.Texture2DArray.MipSlice = mip_slice;
		dsv_desc.Flags = is_read_only ? D3D11_DSV_READ_ONLY_DEPTH : 0;

		ZRY_Result zr{ S_OK };

		zr.value = m_device->CreateDepthStencilView(get_tex_2d_pointer(*tex_handle), &dsv_desc, &m_dsv_resources.at(m_dsv_count));
		RETURN_IF_FAILED_ZRY(zr);
		dsv_handle->index = m_dsv_count;
		m_dsv_count += 1;

		return zr;
	}

	ZRY_Result DX11_Render_Device::create_rtv_tex_2d_array(Render_RTV_Handle* rtv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int array_size, int mip_slice, int first_array_slice)
	{
		assert(rtv_handle != nullptr);

		D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
		ZeroMemory(&rtv_desc, sizeof(rtv_desc));

		rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		rtv_desc.Format = format.value;
		rtv_desc.Texture2DArray.ArraySize = array_size;
		rtv_desc.Texture2DArray.FirstArraySlice = first_array_slice;
		rtv_desc.Texture2DArray.MipSlice = mip_slice;

		ZRY_Result zr{ S_OK };
		zr.value = m_device->CreateRenderTargetView(get_tex_2d_pointer(*tex_handle), &rtv_desc, &m_rtv_resources.at(m_rtv_count));
		RETURN_IF_FAILED_ZRY(zr);
		rtv_handle->index = m_rtv_count;
		m_rtv_count += 1;

		return zr;
	}

	ZRY_Result DX11_Render_Device::create_constant_buffer(Constant_Buffer_Handle* hnd, const D3D11_BUFFER_DESC* buffer_desc)
	{
		zassert((buffer_desc->BindFlags & D3D11_BIND_CONSTANT_BUFFER) != 0);
		Constant_Buffer& const_buffer = m_cb_resources.at(m_const_buff_count);
		hnd->index = m_const_buff_count;
		m_const_buff_count += 1;

		HRESULT hr = m_device->CreateBuffer(buffer_desc, nullptr, &const_buffer.buffer);

		return ZRY_Result{ hr };
	}

	ZRY_Result DX11_Render_Device::create_pso(PSO_Handle* pso_hnd, const PSO_Desc& pso_desc)
	{
		uint64_t pso_hash = hash(pso_desc);
		if (m_pso_handles.find(pso_hash) == m_pso_handles.end())
		{
			pso_hnd->index = m_pso_count;
			m_pso_handles.insert({ pso_hash, *pso_hnd });
			m_pso_count += 1;
			
			Pipeline_State_Object& pso = m_pso_resources.at(pso_hnd->index);
			const auto ps_hnd = shader_manager.ps_hnd_from_bytecode(pso_desc.pixel_shader_bytecode);
			pso.pixel_shader = get_ps_pointer(ps_hnd);

			const auto vs_hnd = shader_manager.vs_hnd_from_bytecode(pso_desc.vertex_shader_bytecode);
			pso.vertex_shader = get_vs_pointer(vs_hnd);

			const auto ds_state_hnd = ds_state_hnd_from_desc(pso_desc.depth_stencil_desc);
			pso.ds_state = get_ds_state_pointer(ds_state_hnd);
			pso.stencil_ref_value = pso_desc.stencil_ref_value;
			
			const auto rs_state_hnd = rs_state_hnd_from_desc(pso_desc.rasterizer_desc);
			pso.rs_state = get_rs_state_pointer(rs_state_hnd);

			const auto bl_state_hnd = bl_state_hnd_from_desc(pso_desc.blend_desc);
			pso.blend_state = get_bl_state_pointer(bl_state_hnd);


			if (pso_desc.num_elements > 0)
			{
				auto& input_layout = m_input_layout_resources.at(m_input_layout_count);
				HRESULT hr = m_device->CreateInputLayout(pso_desc.input_elements_desc, pso_desc.num_elements, pso_desc.vertex_shader_bytecode.bytecode, pso_desc.vertex_shader_bytecode.size_in_bytes, &input_layout);
				zassert(!FAILED(hr));
				pso.input_layout = input_layout;
				m_input_layout_count += 1;
			}

		} else
		{
			*pso_hnd = m_pso_handles.at(pso_hash);
		}

		return ZRY_Result{S_OK};
	}

	ZRY_Result DX11_Render_Device::create_pixel_shader(Pixel_Shader_Handle* ps_hnd, const Shader_Bytecode& bytecode)
	{
		HRESULT hr = m_device->CreatePixelShader(bytecode.bytecode, bytecode.size_in_bytes, nullptr, &m_ps_resources.at(m_ps_count));
		if (!FAILED(hr))
		{
			ps_hnd->index = m_ps_count;
			m_ps_count += 1;
		} 
		else
		{
			ps_hnd->index = 0;
		}

		return ZRY_Result{ hr };
	}

	ZRY_Result DX11_Render_Device::create_vertex_shader(Vertex_Shader_Handle* vs_hnd, const Shader_Bytecode& bytecode)
	{
		HRESULT hr = m_device->CreateVertexShader(bytecode.bytecode, bytecode.size_in_bytes, nullptr, &m_vs_resources.at(m_vs_count));
		if (!FAILED(hr))
		{
			vs_hnd->index = m_vs_count;
			m_vs_count += 1;
		} else
		{
			vs_hnd->index = 0;
		}

		return ZRY_Result{ hr };
	}

	ZRY_Result DX11_Render_Device::create_ds_state(DS_State_Handle* ds_state_hnd, const D3D11_DEPTH_STENCIL_DESC& ds_state_desc)
	{
		HRESULT hr = m_device->CreateDepthStencilState(&ds_state_desc, &m_ds_state_resources.at(m_ds_state_count));
		if (!FAILED(hr))
		{
			ds_state_hnd->index = m_ds_state_count;
			m_ds_state_count += 1;
		} else
		{
			ds_state_hnd->index = 0;
		}

		return ZRY_Result{ hr };
	}

	ZRY_Result DX11_Render_Device::create_rs_state(RS_State_Handle* rs_state_hnd, const D3D11_RASTERIZER_DESC& rs_state_desc)
	{
		HRESULT hr = m_device->CreateRasterizerState(&rs_state_desc, &m_rs_state_resources.at(m_rs_state_count));
		if (!FAILED(hr))
		{
			rs_state_hnd->index = m_rs_state_count;
			m_rs_state_count += 1;
		} else
		{
			rs_state_hnd->index = 0;
		}

		return ZRY_Result{ hr };
	}

	ZRY_Result DX11_Render_Device::create_bl_state(BL_State_Handle* bl_state_hnd, const D3D11_BLEND_DESC& bl_state_desc)
	{
		HRESULT hr = m_device->CreateBlendState(&bl_state_desc, &m_bl_state_resources.at(m_bl_state_count));
		if (!FAILED(hr))
		{
			bl_state_hnd->index = m_bl_state_count;
			m_bl_state_count += 1;
		} else
		{
			bl_state_hnd->index = 0;
		}

		return ZRY_Result{ hr };
	}

	Render_SRV_Handle DX11_Render_Device::add_srv(ID3D11ShaderResourceView*&& srv_resource)
	{
		m_srv_resources.at(m_srv_count) = srv_resource;
		srv_resource = nullptr;
		Render_SRV_Handle hnd_srv{ (uint32_t)m_srv_count };
		m_srv_count += 1;

		return hnd_srv;
	}

	ID3D11Texture2D* DX11_Render_Device::get_tex_2d_pointer(const Render_Texture_Handle rt_hnd) const
	{
		ID3D11Texture2D* resource = m_tex_2d_resources.at(rt_hnd.index);
		return resource;
	}

	ID3D11RenderTargetView* DX11_Render_Device::get_rtv_pointer(const Render_RTV_Handle rtv_hnd) const
	{
		ID3D11RenderTargetView* resource = m_rtv_resources.at(rtv_hnd.index);
		return resource;
	}

	ID3D11ShaderResourceView* DX11_Render_Device::get_srv_pointer(const Render_SRV_Handle srv_hnd) const
	{
		ID3D11ShaderResourceView* resource = m_srv_resources.at(srv_hnd.index);
		return resource;
	}

	ID3D11DepthStencilView* DX11_Render_Device::get_dsv_pointer(const Render_DSV_Handle dsv_hnd) const
	{
		ID3D11DepthStencilView* resource = m_dsv_resources.at(dsv_hnd.index);
		return resource;
	}

	ID3D11Buffer* DX11_Render_Device::get_cb_pointer(const Constant_Buffer_Handle cb_hnd) const
	{
		ID3D11Buffer* buffer = m_cb_resources.at(cb_hnd.index).buffer;
		return buffer;
	}

	ID3D11PixelShader* DX11_Render_Device::get_ps_pointer(const Pixel_Shader_Handle ps_hnd) const
	{
		ID3D11PixelShader* ps = m_ps_resources.at(ps_hnd.index);
		return ps;
	}

	ID3D11VertexShader* DX11_Render_Device::get_vs_pointer(const Vertex_Shader_Handle vs_hnd) const
	{
		ID3D11VertexShader* vs = m_vs_resources.at(vs_hnd.index);
		return vs;
	}

	ID3D11DepthStencilState* DX11_Render_Device::get_ds_state_pointer(const DS_State_Handle ds_hnd) const
	{
		ID3D11DepthStencilState* ds_state = m_ds_state_resources.at(ds_hnd.index);
		return ds_state;
	}

	ID3D11RasterizerState* DX11_Render_Device::get_rs_state_pointer(const RS_State_Handle rs_hnd) const
	{
		ID3D11RasterizerState* rs_state = m_rs_state_resources.at(rs_hnd.index);
		return rs_state;
	}

	ID3D11BlendState* DX11_Render_Device::get_bl_state_pointer(const BL_State_Handle bl_hnd) const
	{
		ID3D11BlendState* bl_state = m_bl_state_resources.at(bl_hnd.index);
		return bl_state;
	}

	const Pipeline_State_Object* DX11_Render_Device::get_pso_pointer(const PSO_Handle pso_hnd) const
	{
		const Pipeline_State_Object* pso = &m_pso_resources.at(pso_hnd.index);
		return pso;
	}

	DS_State_Handle DX11_Render_Device::ds_state_hnd_from_desc(const D3D11_DEPTH_STENCIL_DESC& ds_state_desc)
	{
		uint64_t ds_hash = XXH64(&ds_state_desc, sizeof(ds_state_desc), 0);
		auto found_it = m_ds_state_handles.find(ds_hash);
		DS_State_Handle ds_state_hnd{ 0 };

		if (found_it == m_ds_state_handles.end())
		{
			auto err = create_ds_state(&ds_state_hnd, ds_state_desc);
			zassert(err.value == S_OK);
			m_ds_state_handles.insert({ ds_hash, ds_state_hnd });
		} else
		{
			ds_state_hnd = found_it->second;
		}

		return ds_state_hnd;
	}

	RS_State_Handle DX11_Render_Device::rs_state_hnd_from_desc(const D3D11_RASTERIZER_DESC& rs_state_desc)
	{
		uint64_t rs_hash = XXH64(&rs_state_desc, sizeof(rs_state_desc), 0);
		auto found_it = m_rs_state_handles.find(rs_hash);
		RS_State_Handle rs_state_hnd{ 0 };

		if (found_it == m_rs_state_handles.end())
		{
			auto err = create_rs_state(&rs_state_hnd, rs_state_desc);
			zassert(err.value == S_OK);
			m_rs_state_handles.insert({ rs_hash, rs_state_hnd });
		} else
		{
			rs_state_hnd = found_it->second;
		}

		return rs_state_hnd;
	}

	BL_State_Handle DX11_Render_Device::bl_state_hnd_from_desc(const D3D11_BLEND_DESC& bl_state_desc)
	{
		uint64_t bl_hash = XXH64(&bl_state_desc, sizeof(bl_state_desc), 0);
		auto found_it = m_bl_state_handles.find(bl_hash);
		BL_State_Handle bl_state_hnd{ 0 };

		if (found_it == m_bl_state_handles.end())
		{
			auto err = create_bl_state(&bl_state_hnd, bl_state_desc);
			zassert(err.value == S_OK);
			m_bl_state_handles.insert({ bl_hash, bl_state_hnd });
		} else
		{
			bl_state_hnd = found_it->second;
		}

		return bl_state_hnd;
	}

	void DX11_Render_Device::release_all_resources()
	{
		for (size_t i = 0; i < m_srv_count; i++)
		{
			ID3D11ShaderResourceView* resource = m_srv_resources.at(i);
			if (resource)resource->Release();
		}
		m_srv_count = 0;

		for (size_t i = 0; i < m_rtv_count; i++)
		{
			ID3D11RenderTargetView* resource = m_rtv_resources.at(i);
			if (resource)resource->Release();
		}
		m_rtv_count = 0;

		for (size_t i = 0; i < m_dsv_count; i++)
		{
			ID3D11DepthStencilView* resource = m_dsv_resources.at(i);
			if (resource)resource->Release();
		}
		m_dsv_count = 0;

		for (size_t i = 0; i < m_tex_2d_count; i++)
		{
			ID3D11Texture2D* resource = m_tex_2d_resources.at(i);
			if (resource)resource->Release();
		}
		m_tex_2d_count = 0;

		for (size_t i = 0; i < m_const_buff_count; i++)
		{
			Constant_Buffer& resource = m_cb_resources.at(i);
			if (resource.buffer) resource.buffer->Release();
		}
		m_const_buff_count = 0;
		
		for (size_t i = 0; i < m_ps_count; i++)
		{
			auto& resource = m_ps_resources.at(i);
			if (resource) resource->Release();
		}
		m_ps_count = 0;
				
		for (size_t i = 0; i < m_vs_count; i++)
		{
			auto& resource = m_vs_resources.at(i);
			if (resource) resource->Release();
		}
		m_vs_count = 0;
				
		for (size_t i = 0; i < m_ds_state_count; i++)
		{
			auto& resource = m_ds_state_resources.at(i);
			if (resource) resource->Release();
		}
		m_ds_state_count = 0;
				
		for (size_t i = 0; i < m_rs_state_count; i++)
		{
			auto& resource = m_rs_state_resources.at(i);
			if (resource) resource->Release();
		}
		m_rs_state_count = 0;

		for (size_t i = 0; i < m_bl_state_count; i++)
		{
			auto& resource = m_bl_state_resources.at(i);
			if (resource) resource->Release();
		}
		m_bl_state_count = 0;
		
		for (size_t i = 0; i < m_input_layout_count; i++)
		{
			auto& resource = m_input_layout_resources.at(i);
			if (resource)
			{
				auto a = resource->Release();
			}
		}
		m_input_layout_count = 0;

	}

}