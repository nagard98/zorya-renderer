#include <d3d11_1.h>
#include "imgui_impl_dx11.h"

#include <iostream>
#include <cstdint>

#include "editor/Logger.h"

#include "renderer/backend/rhi/RenderDevice.h"
#include "renderer/backend/rhi/RHIState.h"
#include "renderer/backend/rhi/RenderHardwareInterface.h"
#include "renderer/backend/GraphicsContext.h"

#include "renderer/frontend/Material.h"
#include "renderer/frontend/Asset.h"
#include "ApplicationConfig.h"

#include "WICTextureLoader.h"


namespace zorya
{
	namespace dx = DirectX;

	Render_Hardware_Interface rhi;

	Render_Hardware_Interface::Render_Hardware_Interface() {}
	Render_Hardware_Interface::~Render_Hardware_Interface() {}

	HRESULT Render_Hardware_Interface::init(HWND window_handle, RHI_State initial_state)
	{
		gfx_root_signature.root_params[0].base_register = 0;
		gfx_root_signature.root_params[0].num_resources = 10;
		gfx_root_signature.root_params[0].view_type = Shader_View_Type::CBV;
		gfx_root_signature.root_params[0].visibility = Shader_Visibility::VERTEX_SHADER;
		
		gfx_root_signature.root_params[1].base_register = 0;
		gfx_root_signature.root_params[1].num_resources = 10;
		gfx_root_signature.root_params[1].view_type = Shader_View_Type::SRV;
		gfx_root_signature.root_params[1].visibility = Shader_Visibility::VERTEX_SHADER;
		
		gfx_root_signature.root_params[2].base_register = 0;
		gfx_root_signature.root_params[2].num_resources = 10;
		gfx_root_signature.root_params[2].view_type = Shader_View_Type::CBV;
		gfx_root_signature.root_params[2].visibility = Shader_Visibility::PIXEL_SHADER;
		
		gfx_root_signature.root_params[3].base_register = 0;
		gfx_root_signature.root_params[3].num_resources = 10;
		gfx_root_signature.root_params[3].view_type = Shader_View_Type::SRV;
		gfx_root_signature.root_params[3].visibility = Shader_Visibility::PIXEL_SHADER;

		HRESULT h_res = S_OK;

		RECT rect;
		GetClientRect(window_handle, &rect);
		UINT width = rect.right - rect.left;
		UINT height = rect.bottom - rect.top;

		UINT device_flags = 0;
#if _DEBUG
		device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		D3D_DRIVER_TYPE d3d_driver_supp[] = {
			D3D_DRIVER_TYPE_HARDWARE,
			D3D_DRIVER_TYPE_WARP,
			D3D_DRIVER_TYPE_REFERENCE
		};
		UINT driver_types_num = ARRAYSIZE(d3d_driver_supp);

		D3D_FEATURE_LEVEL d3d_feature_level_supp[] = {
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0
			/*D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0*/
		};
		UINT number_feature_levels = ARRAYSIZE(d3d_feature_level_supp);

		DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
		SecureZeroMemory(&swap_chain_desc, sizeof(swap_chain_desc));

		swap_chain_desc.BufferDesc.Width = width;
		swap_chain_desc.BufferDesc.Height = height;
		swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
		swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
		swap_chain_desc.SampleDesc.Count = MULTISAMPLE_COUNT;
		swap_chain_desc.SampleDesc.Quality = 0;
		swap_chain_desc.Windowed = TRUE;
		swap_chain_desc.OutputWindow = window_handle;
		swap_chain_desc.BufferCount = 1;
		swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;

		for (UINT driver_index = 0; driver_index < driver_types_num; driver_index++)
		{
			h_res = D3D11CreateDeviceAndSwapChain(
				NULL,
				d3d_driver_supp[driver_index],
				NULL,
				device_flags,
				d3d_feature_level_supp,
				number_feature_levels,
				D3D11_SDK_VERSION,
				&swap_chain_desc,
				&m_swap_chain,
				&(m_device.m_device),
				&m_feature_level,
				&m_context
			);

			if (SUCCEEDED(h_res)) break;
		}

		RETURN_IF_FAILED(h_res);


		wrl::ComPtr<IDXGIFactory1> dxgi_factory1;
		{
			wrl::ComPtr<IDXGIDevice> dxgi_device;
			h_res = (m_device.m_device)->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(dxgi_device.GetAddressOf()));

			if (SUCCEEDED(h_res))
			{
				wrl::ComPtr<IDXGIAdapter> adapter;
				h_res = dxgi_device->GetAdapter(adapter.GetAddressOf());

				if (SUCCEEDED(h_res))
				{
					adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(dxgi_factory1.GetAddressOf()));
				}
			}
		}

		RETURN_IF_FAILED(h_res);

		m_device.init();
		m_gfx_context = new GraphicsContext(rhi.m_device.m_device);

		//IDXGIFactory2* dxgiFactory2 = nullptr;
		//hRes = dxgiFactory1->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));

		//if (dxgiFactory2) {
		//    hRes = g_d3dDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&g_d3dDevice1));
		//    if (SUCCEEDED(hRes)) {
		//        (void)g_d3dDeviceContexQueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&g_d3dDeviceContext1));
		//    }

		//    DXGI_SWAP_CHAIN_DESC1 scd1 = {};
		//    SecureZeroMemory(&scd1, sizeof(DXGI_SWAP_CHAIN_DESC1));
		//    scd1.Stereo = FALSE;
		//    scd1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		//    scd1.SampleDesc.Count = 1;
		//    scd1.SampleDesc.Quality = 0;
		//    scd1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		//    //Include front buffer only if fullscreen
		//    scd1.BufferCount = 1;

		//    hRes = dxgiFactory2->CreateSwapChainForHwnd(g_d3dDevice, g_windowHandle, &scd1, nullptr, nullptr, &g_dxgiSwapChain1);
		//    if (SUCCEEDED(hRes)) hRes = g_dxgiSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&g_dxgiSwapChain));

		//    dxgiFactory2->Release();

		//}
		//else {
		//    DXGI_SWAP_CHAIN_DESC scd = {};
		//    SecureZeroMemory(&scd, sizeof(scd));

		//    scd.BufferDesc.Width = width;
		//    scd.BufferDesc.Height = height;
		//    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		//    scd.BufferDesc.RefreshRate.Numerator = 60;
		//    scd.BufferDesc.RefreshRate.Denominator = 1;
		//    scd.SampleDesc.Count = 1;
		//    scd.SampleDesc.Quality = 0;
		//    scd.Windowed = TRUE;
		//    scd.OutputWindow = g_windowHandle;
		//    scd.BufferCount = 1;
		//    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

		//    hRes = dxgiFactory1->CreateSwapChain(g_d3dDevice, &scd, &g_dxgiSwapChain);

		//}

		h_res = dxgi_factory1->MakeWindowAssociation(window_handle, DXGI_MWA_NO_ALT_ENTER);
		RETURN_IF_FAILED(h_res);

		wrl::ComPtr<ID3D11Texture2D> back_buffer;
		h_res = m_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(back_buffer.GetAddressOf()));
		RETURN_IF_FAILED(h_res);

		D3D11_RENDER_TARGET_VIEW_DESC back_buffer_rtv_desc;
		back_buffer_rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		back_buffer_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		back_buffer_rtv_desc.Texture2D.MipSlice = 0;

		h_res = m_device.m_device->CreateRenderTargetView(back_buffer.Get(), nullptr, &m_back_buffer_rtv);
		RETURN_IF_FAILED(h_res);

		m_viewport.TopLeftX = 0;
		m_viewport.TopLeftY = 0;
		m_viewport.Width = (FLOAT)width;
		m_viewport.Height = (FLOAT)height;
		m_viewport.MinDepth = 0.0f;
		m_viewport.MaxDepth = 1.0f;

		//TODO:move sampler creation to somewhere else----------------
		D3D11_SAMPLER_DESC sampler_desc;
		ZeroMemory(&sampler_desc, sizeof(sampler_desc));
		sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		//samplerDesc.BorderColor[0] = 1.0f;
		//samplerDesc.BorderColor[1] = 1.0f;
		//samplerDesc.BorderColor[2] = 1.0f;
		//samplerDesc.BorderColor[3] = 1.0f;
		sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampler_desc.MipLODBias = 0.0f;
		sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampler_desc.MinLOD = 0.0f;
		sampler_desc.MaxLOD = FLT_MAX;
		sampler_desc.MaxAnisotropy = 16;

		RETURN_IF_FAILED(h_res);

		//-----------------------------------------------------------------

		//TODO: move depth/stencil view creation somewhere else------------------

		D3D11_TEXTURE2D_DESC depth_stencil_tex2d_desc{};
		depth_stencil_tex2d_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		depth_stencil_tex2d_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		depth_stencil_tex2d_desc.Width = m_viewport.Width;
		depth_stencil_tex2d_desc.Height = m_viewport.Height;
		depth_stencil_tex2d_desc.MipLevels = 1;
		depth_stencil_tex2d_desc.ArraySize = 1;
		depth_stencil_tex2d_desc.Usage = D3D11_USAGE_DEFAULT;
		depth_stencil_tex2d_desc.SampleDesc.Count = MULTISAMPLE_COUNT;
		depth_stencil_tex2d_desc.SampleDesc.Quality = 0;

		h_res = m_device.m_device->CreateTexture2D(&depth_stencil_tex2d_desc, NULL, &m_back_buffer_depth_tex);
		RETURN_IF_FAILED(h_res);

		D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_dsv_desc;
		depth_stencil_dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depth_stencil_dsv_desc.Flags = 0;
		depth_stencil_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depth_stencil_dsv_desc.Texture2D.MipSlice = 0;

		h_res = m_device.m_device->CreateDepthStencilView(m_back_buffer_depth_tex, &depth_stencil_dsv_desc, &m_back_buffer_depth_dsv);
		RETURN_IF_FAILED(h_res);

		D3D11_SHADER_RESOURCE_VIEW_DESC depth_stencil_srv_desc;
		depth_stencil_srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		depth_stencil_srv_desc.Texture2D.MipLevels = 1;
		depth_stencil_srv_desc.Texture2D.MostDetailedMip = 0;
		depth_stencil_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

		h_res = m_device.m_device->CreateShaderResourceView(m_back_buffer_depth_tex, &depth_stencil_srv_desc, &m_back_buffer_depth_srv);
		RETURN_IF_FAILED(h_res);

		//--------------------------------------------------

		//TODO: move blend desc somewhere else-------------
		D3D11_RENDER_TARGET_BLEND_DESC target_blend_desc1;
		target_blend_desc1.BlendEnable = true;
		target_blend_desc1.SrcBlend = D3D11_BLEND_SRC_ALPHA;
		target_blend_desc1.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		target_blend_desc1.BlendOp = D3D11_BLEND_OP_ADD;
		target_blend_desc1.SrcBlendAlpha = D3D11_BLEND_ONE;
		target_blend_desc1.DestBlendAlpha = D3D11_BLEND_ZERO;
		target_blend_desc1.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		target_blend_desc1.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;


		D3D11_BLEND_DESC blend_desc;
		blend_desc.AlphaToCoverageEnable = false;
		blend_desc.IndependentBlendEnable = false;
		blend_desc.RenderTarget[0] = target_blend_desc1;
		//---------------------------

		D3D11_RENDER_TARGET_BLEND_DESC rtf_blend_desc{};
		rtf_blend_desc.BlendEnable = true;
		rtf_blend_desc.BlendOp = D3D11_BLEND_OP_ADD;
		rtf_blend_desc.SrcBlend = D3D11_BLEND_BLEND_FACTOR;
		rtf_blend_desc.DestBlend = D3D11_BLEND_INV_BLEND_FACTOR;
		rtf_blend_desc.SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		rtf_blend_desc.DestBlendAlpha = D3D11_BLEND_ZERO;
		rtf_blend_desc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		rtf_blend_desc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		D3D11_BLEND_DESC jimenez_gauss_blend_desc{};
		jimenez_gauss_blend_desc.IndependentBlendEnable = false;
		jimenez_gauss_blend_desc.AlphaToCoverageEnable = false;
		jimenez_gauss_blend_desc.RenderTarget[0] = rtf_blend_desc;

		return h_res;
	}

	void Render_Hardware_Interface::shutdown()
	{
		release_all_resources();

		m_context->ClearState();
		m_context->Flush();
		m_debug_layer->Release();

		if (m_context) m_context->Release();
		if (m_device.m_device) m_device.m_device->Release();
	}

	static DXGI_FORMAT convert_format(Format format)
	{
		switch (format)
		{
		case zorya::Format::FORMAT_R8G8B8A8_UNORM:
		case zorya::Format::FORMAT_R8G8B8A8_UNORM_SRGB:
		{
			return DXGI_FORMAT_R8G8B8A8_TYPELESS;
			break;
		}
		case zorya::Format::FORMAT_D32_FLOAT:
		{
			return DXGI_FORMAT_R32_TYPELESS;
			break;
		}
		case zorya::Format::FORMAT_D24_UNORM_S8_UINT:
		{
			return DXGI_FORMAT_R24G8_TYPELESS;
			break;
		}
		case zorya::Format::FORMAT_R16G16_UNORM:
		{
			return DXGI_FORMAT_R16G16_TYPELESS;
			break;
		}
		case zorya::Format::FORMAT_R11G11B10_FLOAT:
		{
			return DXGI_FORMAT_R11G11B10_FLOAT;
			break;
		}
		default:
			zassert(false);
			break;
		}
	}

	static DXGI_FORMAT convert_format(Format format, Bind_Flag bind_flag)
	{
		switch (format)
		{
		case zorya::Format::FORMAT_R8G8B8A8_UNORM:
		{
			switch (bind_flag)
			{
			case zorya::SHADER_RESOURCE:
			case zorya::UNORDERED_ACCESS:
			case zorya::RENDER_TARGET:
			{
				return DXGI_FORMAT_R8G8B8A8_UNORM;
				break;
			}
			case zorya::DEPTH_STENCIL:
			default:
			{
				zassert(false);
				break;
			}
			}
			break;
		}
		case zorya::Format::FORMAT_R8G8B8A8_UNORM_SRGB:
		{
			switch (bind_flag)
			{
			case zorya::SHADER_RESOURCE:
			case zorya::UNORDERED_ACCESS:
			case zorya::RENDER_TARGET:
			{
				return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
				break;
			}
			case zorya::DEPTH_STENCIL:
			default:
			{
				zassert(false);
				break;
			}
			}
			break;
		}
		case zorya::Format::FORMAT_D32_FLOAT:
		{
			switch (bind_flag)
			{
			case zorya::SHADER_RESOURCE:
			case zorya::UNORDERED_ACCESS:
			case zorya::RENDER_TARGET:
			{
				return DXGI_FORMAT_R32_FLOAT;
				break;
			}
			case zorya::DEPTH_STENCIL:
			{
				return DXGI_FORMAT_D32_FLOAT;
				break;
			}
			default:
			{
				zassert(false);
				break;
			}
			}
			break;
		}
		case zorya::Format::FORMAT_D24_UNORM_S8_UINT:
		{
			switch (bind_flag)
			{
			case zorya::SHADER_RESOURCE:
			case zorya::UNORDERED_ACCESS:
			case zorya::RENDER_TARGET:
			{
				return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				break;
			}
			case zorya::DEPTH_STENCIL:
			{
				return DXGI_FORMAT_D24_UNORM_S8_UINT;
				break;
			}
			default:
			{
				zassert(false);
				break;
			}
			}
			break;
		}
		case zorya::Format::FORMAT_R11G11B10_FLOAT:
		{
			switch (bind_flag)
			{
			case zorya::SHADER_RESOURCE:
			case zorya::UNORDERED_ACCESS:
			case zorya::RENDER_TARGET:
			{
				return DXGI_FORMAT_R11G11B10_FLOAT;
				break;
			}
			case zorya::DEPTH_STENCIL:
			default:
			{
				zassert(false);
				break;
			}
			}
			break;
		}
		case zorya::Format::FORMAT_R16G16_UNORM:
		{
			switch (bind_flag)
			{
			case zorya::SHADER_RESOURCE:
			case zorya::UNORDERED_ACCESS:
			case zorya::RENDER_TARGET:
			{
				return DXGI_FORMAT_R16G16_UNORM;
				break;
			}
			case zorya::DEPTH_STENCIL:
			default:
			{
				zassert(false);
				break;
			}
			}
			break;
		}
		default:
		{
			zassert(false);
			break;
		}
		}
	}


	RHI_RESULT Render_Hardware_Interface::load_texture(const wchar_t* path, Shader_Texture2D** shader_texture, bool is_srgb = true, size_t max_size)
	{
		if (*shader_texture != nullptr)
		{
			ID3D11ShaderResourceView* null_views[3] = { nullptr };
			m_context->PSSetShaderResources(0, 3, null_views);

			ID3D11Resource* texture_resource = nullptr;
			(*shader_texture)->GetResource(&texture_resource);

			(*shader_texture)->Release();
			*shader_texture = nullptr;

			texture_resource->Release();
			texture_resource = nullptr;
		}

		HRESULT hRes = dx::CreateWICTextureFromFileEx(m_device.m_device, m_context,
			path,
			max_size,
			D3D11_USAGE_DEFAULT,
			D3D11_BIND_SHADER_RESOURCE,
			0, 0, is_srgb ? dx::WIC_LOADER_SRGB_DEFAULT : dx::WIC_LOADER_IGNORE_SRGB, NULL, shader_texture);

		return hRes == S_OK ? RHI_OK : RHI_ERR;
	}

	RHI_RESULT Render_Hardware_Interface::load_texture2(const Texture2D* const texture_asset, const Texture_Import_Config* const tex_config, Render_SRV_Handle* hnd_srv, size_t max_size)
	{
		assert(texture_asset != nullptr);
		assert(texture_asset->m_is_loaded == true);

		ID3D11ShaderResourceView* shader_texture = get_srv_pointer(*hnd_srv);

		if (shader_texture != nullptr)
		{
			ID3D11ShaderResourceView* null_views[3] = { nullptr };
			m_context->PSSetShaderResources(0, 3, null_views);

			ID3D11Resource* texture_resource = nullptr;
			shader_texture->GetResource(&texture_resource);

			shader_texture->Release();
			shader_texture = nullptr;

			texture_resource->Release();
			texture_resource = nullptr;
		}

		Render_Texture_Handle hnd_staging_tex, hnd_final_tex;
		Render_SRV_Handle hnd_staging_srv/*, hnd_srv_tex*/;

		D3D11_SUBRESOURCE_DATA init_data{};
		init_data.pSysMem = (const void*)texture_asset->m_data;
		//TODO: fix number of channels on import
		init_data.SysMemPitch = tex_config->max_width * 4/*tex_config->channels*/ * (texture_asset->is_hdr ? 4 : 1);
		
		DXGI_FORMAT format = texture_asset->is_hdr ? DXGI_FORMAT_R32G32B32A32_FLOAT : (tex_config->is_normal_map ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);

		ZRY_Result res = m_device.create_tex_2d(&hnd_staging_tex, nullptr, ZRY_Usage{ D3D11_USAGE_DEFAULT }, ZRY_Bind_Flags{ D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET }, ZRY_Format{ format },
			tex_config->max_width, tex_config->max_height, 1,
			&hnd_staging_srv, nullptr, true, 0, 1, 0);
		assert(res.value == S_OK);

		ID3D11Texture2D* stag_tex = m_device.get_tex_2d_pointer(hnd_staging_tex);
		m_context->UpdateSubresource(stag_tex, 0, nullptr, init_data.pSysMem, init_data.SysMemPitch, 0);

		ID3D11ShaderResourceView* stag_srv = m_device.get_srv_pointer(hnd_staging_srv);
		m_context->GenerateMips(stag_srv);

		res = m_device.create_tex_2d(&hnd_final_tex, nullptr, ZRY_Usage{ D3D11_USAGE_DEFAULT}, ZRY_Bind_Flags{ D3D11_BIND_SHADER_RESOURCE }, ZRY_Format{ format },
			tex_config->max_width, tex_config->max_height, 1,
			hnd_srv, nullptr, false, 0, 1, 0);
		assert(res.value == S_OK);

		ID3D11Texture2D* final_tex = m_device.get_tex_2d_pointer(hnd_final_tex);
		m_context->CopyResource(final_tex, stag_tex);

		//TODO : release by removing also handle
		//stag_srv->Release();
		//stag_tex->Release();

		return res.value;
	}


	ZRY_Result Render_Hardware_Interface::create_tex(Render_Texture_Handle* tex_handle, const D3D11_SUBRESOURCE_DATA* init_data, const Render_Graph_Resource_Metadata& meta)
	{
		ZRY_Bind_Flags bind_flags{ 0 };
		bind_flags.value |= (meta.bind_flags & Bind_Flag::RENDER_TARGET) != 0 ? D3D11_BIND_RENDER_TARGET : 0;
		bind_flags.value |= (meta.bind_flags & Bind_Flag::SHADER_RESOURCE) != 0 ? D3D11_BIND_SHADER_RESOURCE : 0;
		bind_flags.value |= (meta.bind_flags & Bind_Flag::UNORDERED_ACCESS) != 0 ? D3D11_BIND_UNORDERED_ACCESS : 0;
		bind_flags.value |= (meta.bind_flags & Bind_Flag::DEPTH_STENCIL) != 0 ? D3D11_BIND_DEPTH_STENCIL : 0;

		ZRY_Result zr{ S_OK };

		if (!meta.desc.is_cubemap)
		{
			zr = m_device.create_tex_2d(
				tex_handle,
				init_data,
				ZRY_Usage{ D3D11_USAGE_DEFAULT },
				bind_flags,
				ZRY_Format{ convert_format(meta.desc.format) },
				meta.desc.width,
				meta.desc.height,
				meta.desc.arr_size,
				nullptr, nullptr,
				meta.desc.has_mips, meta.desc.has_mips ? 0 : 1
			);
		} 
		else
		{
			zr = m_device.create_tex_cubemap(
				tex_handle,
				bind_flags,
				ZRY_Format{ convert_format(meta.desc.format) },
				meta.desc.width,
				meta.desc.height,
				meta.desc.arr_size,
				nullptr, nullptr,
				meta.desc.has_mips, meta.desc.has_mips ? 0 : 1
			);
		}

		return zr;
	}

	ZRY_Result Render_Hardware_Interface::create_srv(Render_SRV_Handle* srv_handle, Render_Texture_Handle* tex_handle, const Render_Graph_Resource_Metadata& meta, const Render_Graph_View_Desc& view_desc)
	{
		u32 slice_size = view_desc.slice_size == 0 ? meta.desc.arr_size : view_desc.slice_size;

		if (!meta.desc.is_cubemap)
		{
			return m_device.create_srv_tex_2d_array(
				srv_handle,
				tex_handle,
				ZRY_Format{ convert_format(meta.desc.format, Bind_Flag::SHADER_RESOURCE) },
				slice_size, view_desc.slice_start_index
			);
		} else
		{
			return m_device.create_srv_tex_cubemap(
				srv_handle,
				tex_handle,
				ZRY_Format{ convert_format(meta.desc.format, Bind_Flag::SHADER_RESOURCE) },
				slice_size, view_desc.slice_start_index
			);
		}


	}

	ZRY_Result Render_Hardware_Interface::create_dsv(Render_DSV_Handle* dsv_handle, Render_Texture_Handle* tex_handle, const Render_Graph_Resource_Metadata& meta, const Render_Graph_View_Desc& view_desc)
	{
		u32 slice_size = view_desc.slice_size == 0 ? meta.desc.arr_size : view_desc.slice_size;

		return m_device.create_dsv_tex_2d_array(
			dsv_handle,
			tex_handle,
			ZRY_Format{ convert_format(meta.desc.format, Bind_Flag::DEPTH_STENCIL) },
			slice_size, 0, view_desc.slice_start_index, view_desc.is_read_only
		);
	}

	ZRY_Result Render_Hardware_Interface::create_rtv(Render_RTV_Handle* rtv_handle, Render_Texture_Handle* tex_handle, const Render_Graph_Resource_Metadata& meta, const Render_Graph_View_Desc& view_desc)
	{
		u32 slice_size = view_desc.slice_size == 0 ? meta.desc.arr_size : view_desc.slice_size;

		return m_device.create_rtv_tex_2d_array(
			rtv_handle,
			tex_handle,
			ZRY_Format{ convert_format(meta.desc.format, Bind_Flag::RENDER_TARGET) },
			slice_size, view_desc.mip_index, view_desc.slice_start_index
		);
	}

	ZRY_Result Render_Hardware_Interface::create_tex_2d(Render_Texture_Handle* tex_handle, const D3D11_SUBRESOURCE_DATA* init_data, ZRY_Usage usage, ZRY_Bind_Flags bind_flags, ZRY_Format format, float width, float height, int array_size, Render_SRV_Handle* srv_handle, Render_RTV_Handle* rtv_handle, bool generate_mips, int mip_levels, int sample_count, int sample_quality)
	{
		ZRY_Result zr = m_device.create_tex_2d(tex_handle, init_data, usage, bind_flags, format, width, height, array_size, srv_handle, rtv_handle, generate_mips, mip_levels, sample_count, sample_quality);
		return zr;
	}

	ZRY_Result Render_Hardware_Interface::create_srv_tex_2d(Render_SRV_Handle* srv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int mip_levels, int most_detailed_mip)
	{
		ZRY_Result zr = m_device.create_srv_tex_2d(srv_handle, tex_handle, format, mip_levels, most_detailed_mip);
		return zr;
	}

	ZRY_Result Render_Hardware_Interface::create_rtv_tex_2d(Render_RTV_Handle* rtv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int mip_slice)
	{
		ZRY_Result zr = m_device.create_rtv_tex_2d(rtv_handle, tex_handle, format, mip_slice);
		return zr;
	}

	ZRY_Result Render_Hardware_Interface::create_dsv_tex_2d(Render_DSV_Handle* dsv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int mip_slice, bool is_read_only)
	{
		ZRY_Result zr = m_device.create_dsv_tex_2d(dsv_handle, tex_handle, format, mip_slice, is_read_only);
		return zr;
	}

	ZRY_Result Render_Hardware_Interface::create_tex_cubemap(Render_Texture_Handle* tex_handle, ZRY_Bind_Flags bind_flags, ZRY_Format format, float width, float height, int array_size, Render_SRV_Handle* srv_handle, Render_RTV_Handle* rtv_handle, bool generate_mips, int mip_levels, int sample_count, int sample_quality)
	{
		ZRY_Result zr = m_device.create_tex_cubemap(tex_handle, bind_flags, format, width, height, array_size, srv_handle, rtv_handle, generate_mips, mip_levels, sample_count, sample_quality);
		return zr;
	}

	ZRY_Result Render_Hardware_Interface::create_srv_tex_2d_array(Render_SRV_Handle* srv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int array_size, int first_array_slice, int mipLevels, int most_detailed_mip)
	{
		ZRY_Result zr = m_device.create_srv_tex_2d_array(srv_handle, tex_handle, format, array_size, first_array_slice, mipLevels, most_detailed_mip );
		return zr;
	}

	ZRY_Result Render_Hardware_Interface::create_dsv_tex_2d_array(Render_DSV_Handle* dsv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int array_size, int mip_slice, int first_array_slice)
	{
		ZRY_Result zr = m_device.create_dsv_tex_2d_array(dsv_handle, tex_handle, format, array_size, mip_slice, first_array_slice);
		return zr;
	}

	ZRY_Result Render_Hardware_Interface::create_constant_buffer(Constant_Buffer_Handle* hnd, const D3D11_BUFFER_DESC* buffer_desc)
	{
		ZRY_Result zr = m_device.create_constant_buffer(hnd, buffer_desc);
		return zr;
	}

	ZRY_Result Render_Hardware_Interface::create_pso(PSO_Handle* pso_hnd, const PSO_Desc& pso_desc)
	{
		ZRY_Result zr = m_device.create_pso(pso_hnd, pso_desc);
		return zr;
	}

	ZRY_Result Render_Hardware_Interface::create_pixel_shader(Pixel_Shader_Handle* ps_hnd, const Shader_Bytecode& bytecode)
	{
		ZRY_Result zr = m_device.create_pixel_shader(ps_hnd, bytecode);
		return zr;
	}

	ZRY_Result Render_Hardware_Interface::create_vertex_shader(Vertex_Shader_Handle* vs_hnd, const Shader_Bytecode& bytecode)
	{
		ZRY_Result zr = m_device.create_vertex_shader(vs_hnd, bytecode);
		return zr;
	}

	ZRY_Result Render_Hardware_Interface::create_ds_state(DS_State_Handle* ds_state_hnd, const D3D11_DEPTH_STENCIL_DESC& ds_state_desc)
	{
		ZRY_Result zr = m_device.create_ds_state(ds_state_hnd, ds_state_desc);
		return zr;
	}

	ZRY_Result Render_Hardware_Interface::create_rs_state(RS_State_Handle* rs_state_hnd, const D3D11_RASTERIZER_DESC& rs_state_desc)
	{
		ZRY_Result zr = m_device.create_rs_state(rs_state_hnd, rs_state_desc);
		return zr;
	}

	Render_SRV_Handle Render_Hardware_Interface::add_srv(ID3D11ShaderResourceView*&& srv_resource)
	{
		return m_device.add_srv(std::forward<ID3D11ShaderResourceView*&&>(srv_resource));
	}

	ID3D11Texture2D* Render_Hardware_Interface::get_tex_2d_pointer(const Render_Texture_Handle rt_hnd) const
	{
		ID3D11Texture2D* res = m_device.get_tex_2d_pointer(rt_hnd);
		return res;
	}

	ID3D11RenderTargetView* Render_Hardware_Interface::get_rtv_pointer(const Render_RTV_Handle rtv_hnd) const
	{
		return m_device.get_rtv_pointer(rtv_hnd);
	}

	ID3D11ShaderResourceView* Render_Hardware_Interface::get_srv_pointer(const Render_SRV_Handle srv_hnd) const
	{
		return m_device.get_srv_pointer(srv_hnd);
	}

	ID3D11DepthStencilView* Render_Hardware_Interface::get_dsv_pointer(const Render_DSV_Handle dsv_hnd) const
	{
		return m_device.get_dsv_pointer(dsv_hnd);
	}

	ID3D11Buffer* Render_Hardware_Interface::get_cb_pointer(const Constant_Buffer_Handle cb_hnd) const
	{
		return m_device.get_cb_pointer(cb_hnd);
	}

	ID3D11PixelShader* Render_Hardware_Interface::get_ps_pointer(const Pixel_Shader_Handle ps_hnd) const
	{
		return m_device.get_ps_pointer(ps_hnd);
	}

	ID3D11VertexShader* Render_Hardware_Interface::get_vs_pointer(const Vertex_Shader_Handle vs_hnd) const
	{
		return m_device.get_vs_pointer(vs_hnd);
	}

	ID3D11DepthStencilState* Render_Hardware_Interface::get_ds_state_pointer(const DS_State_Handle ds_hnd) const
	{
		return m_device.get_ds_state_pointer(ds_hnd);
	}

	ID3D11RasterizerState* Render_Hardware_Interface::get_rs_state_pointer(const RS_State_Handle rs_hnd) const
	{
		return m_device.get_rs_state_pointer(rs_hnd);
	}

	const Pipeline_State_Object* Render_Hardware_Interface::get_pso_pointer(const PSO_Handle pso_hnd) const
	{
		return m_device.get_pso_pointer(pso_hnd);
	}

	DS_State_Handle Render_Hardware_Interface::ds_state_hnd_from_desc(const D3D11_DEPTH_STENCIL_DESC& ds_state_desc)
	{
		return m_device.ds_state_hnd_from_desc(ds_state_desc);
	}

	RS_State_Handle Render_Hardware_Interface::rs_state_hnd_from_desc(const D3D11_RASTERIZER_DESC& rs_state_desc)
	{
		return m_device.rs_state_hnd_from_desc(rs_state_desc);
	}

	HRESULT Render_Hardware_Interface::resize_window(uint32_t width, uint32_t height)
	{
		HRESULT hr = S_FALSE;

		if (m_back_buffer_rtv != nullptr)
		{
			m_context->OMSetRenderTargets(0, 0, 0);

			// Resizing render target
			m_back_buffer_rtv->Release();

			hr = m_swap_chain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
			RETURN_IF_FAILED(hr);

			ID3D11Texture2D* back_buffer = nullptr;
			hr = m_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buffer);
			RETURN_IF_FAILED(hr);

			hr = m_device.m_device->CreateRenderTargetView(back_buffer, NULL, &m_back_buffer_rtv);
			RETURN_IF_FAILED(hr);

			back_buffer->Release();

			//// Resizing depth stencil
			if (m_back_buffer_depth_srv) m_back_buffer_depth_srv->Release();
			if (m_back_buffer_depth_dsv) m_back_buffer_depth_dsv->Release();
			if (m_back_buffer_depth_tex) m_back_buffer_depth_tex->Release();

			D3D11_TEXTURE2D_DESC depth_stencil_tex_desc{};
			depth_stencil_tex_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
			depth_stencil_tex_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
			depth_stencil_tex_desc.Width = width;
			depth_stencil_tex_desc.Height = height;
			depth_stencil_tex_desc.MipLevels = 1;
			depth_stencil_tex_desc.ArraySize = 1;
			depth_stencil_tex_desc.Usage = D3D11_USAGE_DEFAULT;
			depth_stencil_tex_desc.SampleDesc.Count = MULTISAMPLE_COUNT;
			depth_stencil_tex_desc.SampleDesc.Quality = 0;

			hr = m_device.m_device->CreateTexture2D(&depth_stencil_tex_desc, NULL, &m_back_buffer_depth_tex);
			RETURN_IF_FAILED(hr);

			D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_dsv_desc;
			depth_stencil_dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			depth_stencil_dsv_desc.Flags = 0;
			depth_stencil_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			depth_stencil_dsv_desc.Texture2D.MipSlice = 0;

			hr = m_device.m_device->CreateDepthStencilView(m_back_buffer_depth_tex, &depth_stencil_dsv_desc, &m_back_buffer_depth_dsv);
			RETURN_IF_FAILED(hr);

			D3D11_SHADER_RESOURCE_VIEW_DESC depth_stencil_srv_desc;
			depth_stencil_srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			depth_stencil_srv_desc.Texture2D.MipLevels = 1;
			depth_stencil_srv_desc.Texture2D.MostDetailedMip = 0;
			depth_stencil_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

			hr = m_device.m_device->CreateShaderResourceView(m_back_buffer_depth_tex, &depth_stencil_srv_desc, &m_back_buffer_depth_srv);
			RETURN_IF_FAILED(hr);

			// Setting new viewport
			m_viewport.Width = width;
			m_viewport.Height = height;
			m_viewport.MinDepth = 0.0f;
			m_viewport.MaxDepth = 1.0f;
			m_viewport.TopLeftX = 0.0f;
			m_viewport.TopLeftY = 0.0f;

			m_context->RSSetViewports(1, &m_viewport);
		}

		return hr;
	}

	void Render_Hardware_Interface::release_all_resources()
	{
		m_device.release_all_resources();

		if (m_back_buffer_depth_dsv) m_back_buffer_depth_dsv->Release();
		if (m_back_buffer_depth_srv) m_back_buffer_depth_srv->Release();
		if (m_back_buffer_depth_tex) m_back_buffer_depth_tex->Release();
		if (m_back_buffer_rtv) m_back_buffer_rtv->Release();

		if (m_swap_chain) m_swap_chain->Release();

	}

}