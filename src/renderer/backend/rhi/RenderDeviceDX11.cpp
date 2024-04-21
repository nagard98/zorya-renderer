#include "RenderDevice.h"

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
		m_tex_2d_count(0),
		m_rtv_count(0),
		m_srv_count(0),
		m_dsv_count(0)
	{}

	DX11_Render_Device::~DX11_Render_Device()
	{

	}

	ZRY_Result DX11_Render_Device::create_tex_2d(Render_Texture_Handle* tex_handle, ZRY_Bind_Flags bind_flags, ZRY_Format format, float width, float height, int num_tex, Render_SRV_Handle* srv_handle, Render_RTV_Handle* rtv_handle, bool generate_mips, int mip_levels, int array_size, int sample_count, int sample_quality)
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
		tex_2d_desc.MiscFlags = generate_mips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

		ZRY_Result zr{ S_OK };
		for (int i = 0; i < num_tex; i++)
		{
			zr.value = m_device->CreateTexture2D(&tex_2d_desc, nullptr, &m_tex_2d_resources.at(m_tex_2d_count));
			RETURN_IF_FAILED_ZRY(zr);
			//TODO: better handle creation for resources

			if (srv_handle != nullptr)
			{
				zr.value = m_device->CreateShaderResourceView(m_tex_2d_resources.at(m_tex_2d_count), nullptr, &m_srv_resources.at(m_srv_count));
				RETURN_IF_FAILED_ZRY(zr);
				//TODO: better handle creation for resources
				(srv_handle + i)->index = m_srv_count;
				m_srv_count += 1;
			}
			if (rtv_handle != nullptr)
			{
				zr.value = m_device->CreateRenderTargetView(m_tex_2d_resources.at(m_tex_2d_count), nullptr, &m_rtv_resources.at(m_rtv_count));
				RETURN_IF_FAILED_ZRY(zr);
				//TODO: better handle creation for resources
				(rtv_handle + i)->index = m_rtv_count;
				m_rtv_count += 1;
			}

			(tex_handle + i)->index = m_tex_2d_count;
			m_tex_2d_count += 1;
		}

		return zr;
	}

	ZRY_Result DX11_Render_Device::create_srv_tex_2d(Render_SRV_Handle* srv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int num_SRVs, int mip_levels, int most_detailed_map)
	{
		assert(srv_handle != nullptr);

		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
		ZeroMemory(&srv_desc, sizeof(srv_desc));

		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Format = format.value;
		srv_desc.Texture2D.MipLevels = mip_levels;
		srv_desc.Texture2D.MostDetailedMip = most_detailed_map;

		ZRY_Result zr{ S_OK };
		for (int i = 0; i < num_SRVs; i++)
		{
			zr.value = m_device->CreateShaderResourceView(get_tex_2d_pointer(*(tex_handle + i)), &srv_desc, &m_srv_resources.at(m_srv_count));
			RETURN_IF_FAILED_ZRY(zr);
			(srv_handle + i)->index = m_srv_count;
			m_srv_count += 1;
		}

		return zr;
	}

	ZRY_Result DX11_Render_Device::create_rtv_tex_2d(Render_RTV_Handle* rtv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int num_RTVs, int mip_slice)
	{
		assert(rtv_handle != nullptr);

		D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
		ZeroMemory(&rtv_desc, sizeof(rtv_desc));

		rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtv_desc.Format = format.value;
		rtv_desc.Texture2D.MipSlice = mip_slice;

		ZRY_Result zr{ S_OK };
		for (int i = 0; i < num_RTVs; i++)
		{
			zr.value = m_device->CreateRenderTargetView(get_tex_2d_pointer(*(tex_handle + i)), &rtv_desc, &m_rtv_resources.at(m_rtv_count));
			RETURN_IF_FAILED_ZRY(zr);
			(rtv_handle + i)->index = m_rtv_count;
			m_rtv_count += 1;
		}

		return zr;
	}

	ZRY_Result DX11_Render_Device::create_dsv_tex_2d(Render_DSV_Handle* dsv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int num_DSVs, int mip_slice)
	{
		assert(dsv_handle != nullptr);

		D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
		ZeroMemory(&dsv_desc, sizeof(dsv_desc));

		dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsv_desc.Format = format.value;
		dsv_desc.Texture2D.MipSlice = mip_slice;

		ZRY_Result zr{ S_OK };
		for (int i = 0; i < num_DSVs; i++)
		{
			zr.value = m_device->CreateDepthStencilView(get_tex_2d_pointer(*(tex_handle + i)), &dsv_desc, &m_dsv_resources.at(m_dsv_count));
			RETURN_IF_FAILED_ZRY(zr);
			(dsv_handle + i)->index = m_dsv_count;
			m_dsv_count += 1;
		}

		return zr;
	}

	ZRY_Result DX11_Render_Device::createTexCubemap(Render_Texture_Handle* tex_handle, ZRY_Bind_Flags bind_flags, ZRY_Format format, float width, float height, int num_tex, Render_SRV_Handle* srv_handle, Render_RTV_Handle* rtv_handle, bool generate_mips, int mip_levels, int array_size, int sample_count, int sample_quality)
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
		for (int i = 0; i < num_tex; i++)
		{
			zr.value = m_device->CreateTexture2D(&tex_2d_desc, nullptr, &m_tex_2d_resources.at(m_tex_2d_count));
			RETURN_IF_FAILED_ZRY(zr);
			//TODO: better handle creation for resources

			if (srv_handle != nullptr)
			{
				zr.value = m_device->CreateShaderResourceView(m_tex_2d_resources.at(m_tex_2d_count), nullptr, &m_srv_resources.at(m_srv_count));
				RETURN_IF_FAILED_ZRY(zr);
				//TODO: better handle creation for resources
				(srv_handle + i)->index = m_srv_count;
				m_srv_count += 1;
			}
			if (rtv_handle != nullptr)
			{
				zr.value = m_device->CreateRenderTargetView(m_tex_2d_resources.at(m_tex_2d_count), nullptr, &m_rtv_resources.at(m_rtv_count));
				RETURN_IF_FAILED_ZRY(zr);
				//TODO: better handle creation for resources
				(rtv_handle + i)->index = m_rtv_count;
				m_rtv_count += 1;
			}

			(tex_handle + i)->index = m_tex_2d_count;
			m_tex_2d_count += 1;
		}

		return zr;
	}

	ZRY_Result DX11_Render_Device::createSRVTex2DArray(Render_SRV_Handle* srv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int num_SRVs, int array_size, int first_array_slice, int mip_levels, int most_detailed_map)
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
		for (int i = 0; i < num_SRVs; i++)
		{
			zr.value = m_device->CreateShaderResourceView(get_tex_2d_pointer(*(tex_handle + i)), &srv_desc, &m_srv_resources.at(m_srv_count));
			RETURN_IF_FAILED_ZRY(zr);
			(srv_handle + i)->index = m_srv_count;
			m_srv_count += 1;
		}

		return zr;
	}

	ZRY_Result DX11_Render_Device::createDSVTex2DArray(Render_DSV_Handle* dsv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int array_size, int numDSVs, int mip_slice, int first_array_slice)
	{
		assert(dsv_handle != nullptr);

		D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
		ZeroMemory(&dsv_desc, sizeof(dsv_desc));

		dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		dsv_desc.Format = format.value;
		dsv_desc.Texture2DArray.ArraySize = array_size;
		dsv_desc.Texture2DArray.FirstArraySlice = first_array_slice;
		dsv_desc.Texture2DArray.MipSlice = mip_slice;

		ZRY_Result zr{ S_OK };
		for (int i = 0; i < numDSVs; i++)
		{
			zr.value = m_device->CreateDepthStencilView(get_tex_2d_pointer(*(tex_handle + i)), &dsv_desc, &m_dsv_resources.at(m_dsv_count));
			RETURN_IF_FAILED_ZRY(zr);
			(dsv_handle + i)->index = m_dsv_count;
			m_dsv_count += 1;
		}

		return zr;
	}

	ID3D11Texture2D* DX11_Render_Device::get_tex_2d_pointer(const Render_Texture_Handle rt_hnd) const
	{
		return m_tex_2d_resources.at(rt_hnd.index);
	}

	ID3D11RenderTargetView* DX11_Render_Device::get_rtv_pointer(const Render_RTV_Handle rtv_hnd) const
	{
		return m_rtv_resources.at(rtv_hnd.index);
	}

	ID3D11ShaderResourceView* DX11_Render_Device::get_srv_pointer(const Render_SRV_Handle srv_hnd) const
	{
		return m_srv_resources.at(srv_hnd.index);
	}

	ID3D11DepthStencilView* DX11_Render_Device::get_dsv_pointer(const Render_DSV_Handle dsv_hnd) const
	{
		return m_dsv_resources.at(dsv_hnd.index);;
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
			Constant_Buffer& resource = m_constant_buffers.at(i);
			if (resource.buffer) resource.buffer->Release();
		}

		m_const_buff_count = 0;
	}

}