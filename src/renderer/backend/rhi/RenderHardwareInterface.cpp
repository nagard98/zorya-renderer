#include <d3d11_1.h>
#include "imgui_impl_dx11.h"

#include <iostream>
#include <cstdint>

#include "editor/Logger.h"

#include "renderer/backend/rhi/RenderDevice.h"
#include "renderer/backend/rhi/RHIState.h"
#include "renderer/backend/rhi/RenderHardwareInterface.h"

#include "renderer/frontend/Material.h"

#include "ApplicationConfig.h"

#include "WICTextureLoader.h"

namespace zorya
{
	namespace dx = DirectX;

	Render_Hardware_Interface rhi;

	Render_Hardware_Interface::Render_Hardware_Interface()
	{}

	Render_Hardware_Interface::~Render_Hardware_Interface()
	{}

	HRESULT Render_Hardware_Interface::init(HWND window_handle, RHI_State initial_state)
	{
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

		h_res = m_device.m_device->CreateRenderTargetView(back_buffer.Get(), nullptr, &m_back_buffer_rtv);
		RETURN_IF_FAILED(h_res);


		//Render Target for editor scene window?-----------------------------------------------------------------------
		//
		//RenderTextureHandle hndTexRenderTarget;

		//ZRYResult zRes = device.createTex2D(&hndTexRenderTarget, ZRYBindFlags{ D3D11_BIND_SHADER_RESOURCE }, ZRYFormat{ scd.BufferDesc.Format }, scd.BufferDesc.Width, scd.BufferDesc.Height, 1, nullptr, nullptr, false, 1);
		//RETURN_IF_FAILED(zRes.value);
		///*ID3D11Texture2D* rtTexture = */device.getTex2DPointer(hndTexRenderTarget);

		//RenderSRVHandle hndSrvRenderTarget;
		//zRes = device.createSRVTex2D(&hndSrvRenderTarget, &hndTexRenderTarget, ZRYFormat{ DXGI_FORMAT_R8G8B8A8_UNORM }, 1, -1);
		//RETURN_IF_FAILED(zRes.value);
		//renderTargetShaderResourceView = device.getSRVPointer(hndSrvRenderTarget);

		//D3D11_TEXTURE2D_DESC editorSceneTexDesc;
		//ZeroMemory(&editorSceneTexDesc, sizeof(editorSceneTexDesc));
		//editorSceneTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		//editorSceneTexDesc.ArraySize = 1;
		//editorSceneTexDesc.MipLevels = 1;
		//editorSceneTexDesc.Usage = D3D11_USAGE_DEFAULT;
		//editorSceneTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		//editorSceneTexDesc.Height = height;
		//editorSceneTexDesc.Width = width;
		//editorSceneTexDesc.SampleDesc.Count = MULTISAMPLE_COUNT;
		//editorSceneTexDesc.SampleDesc.Quality = 0;

		//hRes = device.device->CreateTexture2D(&editorSceneTexDesc, NULL, &editorSceneTex);
		//RETURN_IF_FAILED(hRes);

		//hRes = device.device->CreateShaderResourceView(editorSceneTex, NULL, &editorSceneSRV);
		//RETURN_IF_FAILED(hRes);

		//------------------------------------------------------------------------------


		(m_context)->OMSetRenderTargets(1, &m_back_buffer_rtv, nullptr);

		m_viewport.TopLeftX = 0;
		m_viewport.TopLeftY = 0;
		m_viewport.Width = (FLOAT)width;
		m_viewport.Height = (FLOAT)height;
		m_viewport.MinDepth = 0.0f;
		m_viewport.MaxDepth = 1.0f;

		m_context->RSSetViewports(1, &m_viewport);

		set_state(initial_state);

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

		h_res = m_device.m_device->CreateSamplerState(&sampler_desc, &m_sampler_state_variants[0].p_state_object);
		RETURN_IF_FAILED(h_res);

		m_context->PSSetSamplers(0, 1, &m_sampler_state_variants[0].p_state_object);
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

		m_context->OMSetRenderTargets(1, &m_back_buffer_rtv, m_back_buffer_depth_dsv);
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

		h_res = m_device.m_device->CreateBlendState(&blend_desc, &m_blend_state_variants[0].p_state_object);
		RETURN_IF_FAILED(h_res);
		//rhi.context->OMSetBlendState(blendVariants[0].pStateObject, 0, 0);
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

		h_res = m_device.m_device->CreateBlendState(&jimenez_gauss_blend_desc, &m_blend_state_variants[1].p_state_object);
		RETURN_IF_FAILED(h_res);

		return h_res;
	}

	void Render_Hardware_Interface::set_state(RHI_State new_state)
	{
		RHI_State state_changes = new_state ^ state;

		// if state hasn't changed from last update, return
		if (state_changes == 0)
		{
			return;
		}

		// Searches raster state object in cache
		if ((state_changes & RASTER_STATE_MASK) != 0)
		{
			RHI_State new_raster_state = new_state & RASTER_STATE_MASK;

			for (int i = 0; i < m_rast_state_variants.size(); i++)
			{
				if (m_rast_state_variants[i].variant == -1)
				{
					Logger::add_log(Logger::Channel::TRACE, "Building new raster state\n");
					ZeroMemory(&m_tmp_rast_desc, sizeof(m_tmp_rast_desc));
					m_tmp_rast_desc.FillMode = (new_raster_state & RASTER_FILL_MODE_MASK) == RASTER_FILL_MODE_SOLID ? D3D11_FILL_SOLID : D3D11_FILL_WIREFRAME;
					m_tmp_rast_desc.AntialiasedLineEnable = (new_raster_state & RASTER_ANTIALIAS_MASK) == RASTER_ANTIALIAS_MASK;
					m_tmp_rast_desc.CullMode = (new_raster_state & RASTER_CULL_MODE_MASK) == RASTER_CULL_FRONT ? D3D11_CULL_FRONT : D3D11_CULL_BACK;
					m_tmp_rast_desc.MultisampleEnable = (new_raster_state & RASTER_MULTISAMPLE_MASK) == RASTER_MULTISAMPLE_MASK;
					m_tmp_rast_desc.ScissorEnable = (new_raster_state & RASTER_SCISSOR_MASK) == RASTER_SCISSOR_MASK;

					m_rast_state_variants[i].variant = new_raster_state & RASTER_STATE_MASK;
					m_device.m_device->CreateRasterizerState(&m_tmp_rast_desc, &(m_rast_state_variants[i].p_state_object));
					m_context->RSSetState(m_rast_state_variants[i].p_state_object);
					break;
				}
				else if (m_rast_state_variants[i].variant == new_raster_state)
				{
					m_context->RSSetState(m_rast_state_variants[i].p_state_object);
					break;
				}
			}
		}

		if ((state_changes & DEPTH_STENCIL_STATE_MASK) != 0)
		{
			RHI_State new_depth_stencil_state = new_state & DEPTH_STENCIL_STATE_MASK;

			for (int i = 0; i < m_depth_stencil_state_variants.size(); i++)
			{
				if (m_depth_stencil_state_variants[i].variant == -1)
				{
					Logger::add_log(Logger::Channel::TRACE, "Building new depth stancil state\n");
					ZeroMemory(&m_tmp_depth_sten_desc, sizeof(m_tmp_depth_sten_desc));
					m_tmp_depth_sten_desc.DepthEnable = (new_depth_stencil_state & DEPTH_ENABLE_MASK) == DEPTH_ENABLE_MASK;
					m_tmp_depth_sten_desc.DepthWriteMask = (new_depth_stencil_state & DEPTH_WRITE_ENABLE_MASK) == DEPTH_WRITE_ENABLE_MASK ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
					m_tmp_depth_sten_desc.DepthFunc = (new_depth_stencil_state & DEPTH_COMP_MASK) == DEPTH_COMP_LESS ? D3D11_COMPARISON_LESS : D3D11_COMPARISON_LESS_EQUAL;

					m_depth_stencil_state_variants[i].variant = new_depth_stencil_state;
					m_device.m_device->CreateDepthStencilState(&m_tmp_depth_sten_desc, &(m_depth_stencil_state_variants[i].p_state_object));
					m_context->OMSetDepthStencilState(m_depth_stencil_state_variants[i].p_state_object, 0.0f);
					break;
				}
				else if (m_depth_stencil_state_variants[i].variant == new_depth_stencil_state)
				{
					m_context->OMSetDepthStencilState(m_depth_stencil_state_variants[i].p_state_object, 0.0f);
					break;
				}
			}
		}

		state = new_state;

	}

	RHI_RESULT Render_Hardware_Interface::load_texture(const wchar_t* path, Shader_Texture2D& shader_texture, bool is_srgb = true, size_t max_size)
	{
		if (shader_texture.resource_view != nullptr)
		{
			ID3D11ShaderResourceView* null_views[3] = { nullptr };
			m_context->PSSetShaderResources(0, 3, null_views);

			ID3D11Resource* texture_resource = nullptr;
			shader_texture.resource_view->GetResource(&texture_resource);

			shader_texture.resource_view->Release();
			shader_texture.resource_view = nullptr;

			texture_resource->Release();
			texture_resource = nullptr;
		}

		HRESULT hRes = dx::CreateWICTextureFromFileEx(m_device.m_device, m_context,
			path,
			max_size,
			D3D11_USAGE_DEFAULT,
			D3D11_BIND_SHADER_RESOURCE,
			0, 0, is_srgb ? dx::WIC_LOADER_SRGB_DEFAULT : dx::WIC_LOADER_IGNORE_SRGB, NULL, &shader_texture.resource_view);

		return hRes == S_OK ? RHI_OK : RHI_ERR;
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

	ID3D11BlendState* Render_Hardware_Interface::get_blend_state(int index)
	{
		return m_blend_state_variants[index].p_state_object;
	}

	void Render_Hardware_Interface::release_all_resources()
	{
		if (m_back_buffer_depth_dsv) m_back_buffer_depth_dsv->Release();
		if (m_back_buffer_depth_srv) m_back_buffer_depth_srv->Release();
		if (m_back_buffer_depth_tex) m_back_buffer_depth_tex->Release();
		if (m_back_buffer_rtv) m_back_buffer_rtv->Release();

		if (m_swap_chain) m_swap_chain->Release();

		for (auto& rasterizer_state : m_rast_state_variants)
		{
			if (rasterizer_state.p_state_object) rasterizer_state.p_state_object->Release();
		}

		for (auto& depth_stencil_state : m_depth_stencil_state_variants)
		{
			if (depth_stencil_state.p_state_object) depth_stencil_state.p_state_object->Release();
		}

		for (auto& blend_state : m_blend_state_variants)
		{
			if (blend_state.p_state_object) blend_state.p_state_object->Release();
		}

		for (auto& sampler_state : m_sampler_state_variants)
		{
			if (sampler_state.p_state_object) sampler_state.p_state_object->Release();
		}

		m_device.release_all_resources();
	}

}