#ifndef RENDER_DEVICE_H_
#define RENDER_DEVICE_H_

#include <iostream>
#include <cstdint>
#include <d3d11_1.h>
#include <wrl/client.h>
#include <vector>

#include <renderer/backend/ConstantBuffer.h>

namespace zorya
{
	namespace wrl = Microsoft::WRL;

	#define RETURN_IF_FAILED(h_result) { if(FAILED(h_result)) return h_result; }
	#define RETURN_IF_FAILED_ZRY(z_result) { if(FAILED(z_result.value)) return z_result; }

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

	struct Render_Texture_Handle
	{
		uint64_t index;
	};

	struct Render_SRV_Handle
	{
		uint64_t index;
	};

	struct Render_RTV_Handle
	{
		uint64_t index;
	};

	struct Render_DSV_Handle
	{
		uint64_t index;
	};


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

		template <typename T>
		ZRY_Result create_constant_buffer(constant_buffer_handle<T>* hnd_constant_buffer, const char* name)
		{
			D3D11_BUFFER_DESC buffer_desc{};
			buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			buffer_desc.ByteWidth = sizeof(T);

			Constant_Buffer& const_buffer = m_constant_buffers.emplace_back(Constant_Buffer{ nullptr, name });

			HRESULT hr = m_device->CreateBuffer(&buffer_desc, nullptr, &const_buffer.buffer);
			if (!FAILED(hr))
			{
				hnd_constant_buffer->index = m_const_buff_count;
				m_const_buff_count += 1;
			}

			return ZRY_Result{ hr };
		}

		//TODO: move up to rhi
		template <typename T>
		ZRY_Result update_constant_buffer(constant_buffer_handle<T> hnd_constant_buffer, const T& new_buffer_content)
		{
			Constant_Buffer& constant_buffer = m_constant_buffers.at(hnd_constant_buffer.index);
			rhi.m_context->UpdateSubresource(constant_buffer.buffer, 0, nullptr, &new_buffer_content, 0, 0);

			return ZRY_Result{ S_OK };
		}

		ZRY_Result create_tex_2d(Render_Texture_Handle* tex_handle, ZRY_Bind_Flags bind_flags, ZRY_Format format, float width, float height, int numTex = 1, Render_SRV_Handle* srv_handle = nullptr, Render_RTV_Handle* rtv_handle = nullptr, bool generate_mips = false, int mip_levels = 0, int array_size = 1, int sample_count = 1, int sample_quality = 0);
		ZRY_Result create_srv_tex_2d(Render_SRV_Handle* srv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int num_SRVs = 1, int mip_levels = 1, int most_detailed_mip = 0);
		ZRY_Result create_rtv_tex_2d(Render_RTV_Handle* rtv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int num_RTVs = 1, int mip_slice = 0);
		ZRY_Result create_dsv_tex_2d(Render_DSV_Handle* dsv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int num_DSVs = 1, int mip_slice = 0);

		ZRY_Result createTexCubemap(Render_Texture_Handle* tex_handle, ZRY_Bind_Flags bind_flags, ZRY_Format format, float width, float height, int num_tex = 1, Render_SRV_Handle* srv_handle = nullptr, Render_RTV_Handle* rtv_handle = nullptr, bool generate_mips = false, int mip_levels = 0, int array_size = 1, int sample_count = 1, int sample_quality = 0);
		ZRY_Result createSRVTex2DArray(Render_SRV_Handle* srv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int num_SRVs = 1, int array_size = 1, int first_array_slice = 0, int mipLevels = 1, int most_detailed_mip = 0);
		ZRY_Result createDSVTex2DArray(Render_DSV_Handle* dsv_handle, const Render_Texture_Handle* tex_handle, ZRY_Format format, int array_size, int num_DSVs = 1, int mip_slice = 0, int first_array_slice = 0);

		//TODO: remove this method when rest of abstraction is implemented
		ID3D11Texture2D* get_tex_2d_pointer(const Render_Texture_Handle rt_hnd) const;
		ID3D11RenderTargetView* get_rtv_pointer(const Render_RTV_Handle rtv_hnd) const;
		ID3D11ShaderResourceView* get_srv_pointer(const Render_SRV_Handle srv_hnd) const;
		ID3D11DepthStencilView* get_dsv_pointer(const Render_DSV_Handle dsv_hnd) const;

		void release_all_resources();

		//TODO: move to private when you add all the functionality in this abstraction layer
		ID3D11Device* m_device;

		//TODO: move to private; just temporary public before implementing constant buffer binding API
		std::vector<Constant_Buffer> m_constant_buffers;

	private:

		std::vector<ID3D11Texture2D*> m_tex_2d_resources;
		std::vector<ID3D11RenderTargetView*> m_rtv_resources;
		std::vector<ID3D11ShaderResourceView*> m_srv_resources;
		std::vector<ID3D11DepthStencilView*> m_dsv_resources;


		int m_tex_2d_count, m_rtv_count, m_srv_count, m_dsv_count, m_const_buff_count;

	};



}
#endif