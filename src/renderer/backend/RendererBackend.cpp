#include "renderer/backend/RendererBackend.h"
#include "renderer/backend/BufferCache.h"
#include "renderer/backend/ResourceCache.h"
#include "renderer/backend/ConstantBuffer.h"

#include "renderer/backend/rhi/RenderHardwareInterface.h"
#include "renderer/backend/rhi/RHIState.h"
#include "renderer/backend/rhi/RenderDevice.h"

#include "renderer/frontend/RendererFrontend.h"
#include "renderer/frontend/Material.h"
#include "renderer/frontend/Lights.h"
#include "renderer/frontend/Camera.h"
#include "renderer/frontend/Shader.h"

#include "shaders/common/LightStruct.hlsli"

#include "ApplicationConfig.h"

#include <DDSTextureLoader.h>

#include <d3d11_1.h>
#include <vector>
#include <cassert>
#include <fstream>
#include <iostream>


namespace zorya
{
	Renderer_Backend rb;

	Renderer_Backend::Renderer_Backend()
	{
		m_shadow_map_viewport = D3D11_VIEWPORT{};
		m_scene_viewport = D3D11_VIEWPORT{};

		//matPrmsCB = nullptr;
		//lightsCB = nullptr;
		m_object_cb = nullptr;
		m_proj_cb = nullptr;
		m_view_cb = nullptr;
		m_dir_shad_cb = nullptr;
		m_omni_dir_shad_cb = nullptr;

		m_shadow_map = nullptr;
		m_shadow_map_dsv = nullptr;
		m_shadow_map_srv = nullptr;

		m_cubemap_view = nullptr;

		m_shadow_cube_map = nullptr;
		m_shadow_cube_map_srv = nullptr;
		for (int i = 0; i < 6; i++)
		{
			m_shadow_cube_map_dsv[i] = nullptr;
		}

		for (int i = 0; i < 5; i++)
		{
			m_skin_rt[i] = nullptr;
			m_skin_maps[i] = nullptr;
			m_skin_srv[i] = nullptr;
		}

		for (int i = 0; i < G_Buffer::SIZE; i++)
		{
			m_gbuffer_srv[i] = nullptr;
			m_gbuffer_rtv[i] = nullptr;
			m_gbuffer[i] = nullptr;
		}

		m_inv_mat_cb = nullptr;

		m_annot = nullptr;
	}


	Renderer_Backend::~Renderer_Backend()
	{}

	void Renderer_Backend::release_all_resources()
	{
		//if (matPrmsCB) matPrmsCB->Release();
		//if (lightsCB) lightsCB->Release();
		if (m_object_cb) m_object_cb->Release();
		if (m_view_cb) m_view_cb->Release();
		if (m_proj_cb) m_proj_cb->Release();
		if (m_dir_shad_cb) m_dir_shad_cb->Release();
		if (m_omni_dir_shad_cb) m_omni_dir_shad_cb->Release();

		if (m_inv_mat_cb) m_inv_mat_cb->Release();
		if (m_annot) m_annot->Release();

		if (m_thickness_map_srv.resource_view) m_thickness_map_srv.resource_view->Release();
		if (m_cubemap_view) m_cubemap_view->Release();

		m_skybox_pixel_shader.free_shader();
		m_skybox_vertex_shader.free_shader();

		m_shadow_map_vertex_shader.free_shader();
		m_shadow_map_pixel_shader.free_shader();

		m_gbuffer_vertex_shader.free_shader();
		m_fullscreen_quad_shader.free_shader();

		m_lighting_shader.free_shader();

		m_sssss_pixel_shader.free_shader();
	}

	struct Kernel_Sample
	{
		float r, g, b, x, y;
	};

	float num_samples = 15;
	std::vector<dx::XMFLOAT4> kernel;

	void load_kernel_file(std::string fileName, std::vector<float>& data)
	{
		//data.clear();

		//std::string folder(kernelFolder.begin(), kernelFolder.end()); // dirty conversion
		std::string path = fileName;//folder + fileName;

		bool binary = false;
		if (fileName.compare(fileName.size() - 3, 3, ".bn") == 0)
			binary = true;

		std::ifstream i;
		std::ios_base::openmode om;

		if (binary) om = std::ios_base::in | std::ios_base::binary;
		else om = std::ios_base::in;

		i.open(path, om);

		if (!i.good())
		{
			i.close();
			i.open("../" + path, om);
		}

		if (binary)
		{
			data.clear();

			// read float count
			char sv[4];
			i.read(sv, 4);
			int fc = (int)(floor(*((float*)sv)));

			data.resize(fc);
			i.read(reinterpret_cast<char*>(&data[0]), fc * 4);
		}
		else
		{
			float v;

			while (i >> v)
			{
				data.push_back(v);

				int next = i.peek();
				switch (next)
				{
				case ',': i.ignore(1); break;
				case ' ': i.ignore(1); break;
				}
			}
		}

		i.close();
	}

	void calculate_offset(float _range, float _exponent, int _offsetCount, std::vector<float>& _offsets)
	{
		// Calculate the offsets:
		float step = 2.0f * _range / (_offsetCount - 1);
		for (int i = 0; i < _offsetCount; i++)
		{
			float o = -_range + float(i) * step;
			float sign = o < 0.0f ? -1.0f : 1.0f;
			float ofs = _range * sign * abs(pow(o, _exponent)) / pow(_range, _exponent);
			_offsets.push_back(ofs);
		}
	}

	void calculate_areas(std::vector<float>& _offsets, std::vector<float>& _areas)
	{
		int size = _offsets.size();

		for (int i = 0; i < size; i++)
		{
			float w0 = i > 0 ? abs(_offsets[i] - _offsets[i - 1]) : 0.0f;
			float w1 = i < size - 1 ? abs(_offsets[i] - _offsets[i + 1]) : 0.0f;
			float area = (w0 + w1) / 2.0f;
			_areas.push_back(area);
		}
	}

	dx::XMFLOAT3 lin_interpol_1d(std::vector<Kernel_Sample> _kernelData, float _x)
	{
		// naive, a lot to improve here

		if (_kernelData.size() < 1) throw "_kernelData empty";

		unsigned int i = 0;
		while (i < _kernelData.size())
		{
			if (_x > _kernelData[i].x) i++;
			else break;
		}

		dx::XMFLOAT3 v;

		if (i < 1)
		{
			v.x = _kernelData[0].r;
			v.y = _kernelData[0].g;
			v.z = _kernelData[0].b;
		}
		else if (i > _kernelData.size() - 1)
		{
			v.x = _kernelData[_kernelData.size() - 1].r;
			v.y = _kernelData[_kernelData.size() - 1].g;
			v.z = _kernelData[_kernelData.size() - 1].b;
		}
		else
		{
			Kernel_Sample b = _kernelData[i];
			Kernel_Sample a = _kernelData[i - 1];

			float d = b.x - a.x;
			float dx = _x - a.x;

			float t = dx / d;

			v.x = a.r * (1 - t) + b.r * t;
			v.y = a.g * (1 - t) + b.g * t;
			v.z = a.b * (1 - t) + b.b * t;
		}

		return v;
	}

	void calculate_ssss_discr_sep_kernel(const std::vector<Kernel_Sample>& _kernelData)
	{
		const float EXPONENT = 2.0f; // used for impartance sampling

		float RANGE = _kernelData[_kernelData.size() - 1].x; // get max. sample location

		// calculate offsets
		std::vector<float> offsets;
		calculate_offset(RANGE, EXPONENT, num_samples, offsets);

		// calculate areas (using importance-sampling) 
		std::vector<float> areas;
		calculate_areas(offsets, areas);

		kernel.resize(num_samples);

		dx::XMFLOAT3 sum = dx::XMFLOAT3(0, 0, 0); // weights sum for normalization

		// compute interpolated weights
		for (int i = 0; i < num_samples; i++)
		{
			float sx = offsets[i];

			dx::XMFLOAT3 v = lin_interpol_1d(_kernelData, sx);
			kernel[i].x = v.x * areas[i];
			kernel[i].y = v.y * areas[i];
			kernel[i].z = v.z * areas[i];
			kernel[i].w = sx;

			sum.x += kernel[i].x;
			sum.y += kernel[i].y;
			sum.z += kernel[i].z;
		}

		// Normalize
		for (int i = 0; i < num_samples; i++)
		{
			kernel[i].x /= sum.x;
			kernel[i].y /= sum.y;
			kernel[i].z /= sum.z;
		}

		// TEMP put center at first
		dx::XMFLOAT4 t = kernel[num_samples / 2];
		for (int i = num_samples / 2; i > 0; i--)
			kernel[i] = kernel[i - 1];
		kernel[0] = t;

		// set shader vars
		//HRESULT hr;
		//V(effect->GetVariableByName("maxOffsetMm")->AsScalar()->SetFloat(RANGE));
		//V(kernelVariable->SetFloatVectorArray((float*)&kernel.front(), 0, nSamples));
	}

	void override_ssss_discr_sep_kernel(const std::vector<float>& _kernel_data)
	{
		bool use_img_2d_kernel = false;

		// conversion of linear kernel data to sample array
		std::vector<Kernel_Sample> k;
		unsigned int size = _kernel_data.size() / 4;

		unsigned int i = 0;
		for (unsigned int s = 0; s < size; s++)
		{
			Kernel_Sample ks;
			ks.r = _kernel_data[i++];
			ks.g = _kernel_data[i++];
			ks.b = _kernel_data[i++];
			ks.x = _kernel_data[i++];
			k.push_back(ks);
		}

		// kernel computation
		calculate_ssss_discr_sep_kernel(k);
	}

	dx::XMFLOAT3 gauss_1D(float x, dx::XMFLOAT3 variance)
	{
		dx::XMVECTOR var = dx::XMLoadFloat3(&variance);
		dx::XMVECTOR var2 = dx::XMVectorMultiply(var, var);
		dx::XMVECTOR var2_2 = dx::XMVectorAdd(var2, var2);
		dx::XMVECTOR x_vec = dx::XMVectorMultiply(dx::XMVectorSet(x, x, x, 0.0f), dx::XMVectorSet(x, x, x, 0.0f));
		dx::XMVECTOR neg_x_vec = dx::XMVectorNegate(x_vec);

		dx::XMVECTOR res = dx::XMVectorMultiply(dx::XMVectorMultiply(dx::XMVectorReciprocal(var), dx::XMVectorReciprocalSqrt(dx::XMVectorAdd(dx::g_XMPi, dx::g_XMPi))), dx::XMVectorExp(dx::XMVectorMultiply(neg_x_vec, dx::XMVectorReciprocal(var2_2))));
		dx::XMFLOAT3 gs1D{};
		dx::XMStoreFloat3(&gs1D, res);

		return gs1D;
		//return rcp(sqrt(2.0f * dx::XM_PI) * variance) * exp(-(x * x) * rcp(2.0f * variance * variance));
	}

	dx::XMFLOAT3 profile(float r)
	{
		/**
		 * We used the red channel of the original skin profile defined in
		 * [d'Eon07] for all three channels. We noticed it can be used for green
		 * and blue channels (scaled using the falloff parameter) without
		 * introducing noticeable differences and allowing for total control over
		 * the profile. For example, it allows to create blue SSS gradients, which
		 * could be useful in case of rendering blue creatures.
		 */
		 //return  // 0.233f * gaussian(0.0064f, r) + /* We consider this one to be directly bounced light, accounted by the strength parameter (see @STRENGTH) */
	   /*      0.100f * gaussian(0.0484f, r) +
			 0.118f * gaussian(0.187f, r) +
			 0.113f * gaussian(0.567f, r) +
			 0.358f * gaussian(1.99f, r) +
			 0.078f * gaussian(7.41f, r);*/
		dx::XMFLOAT3 near_var = dx::XMFLOAT3(0.077f, 0.034f, 0.02f);
		dx::XMFLOAT3 far_var = dx::XMFLOAT3(1.0f, 0.45f, 0.25f);

		dx::XMFLOAT3 g1_float = (gauss_1D(r, near_var));
		dx::XMFLOAT3 g2_float = (gauss_1D(r, far_var));
		dx::XMVECTOR g1 = dx::XMLoadFloat3(&g1_float);
		dx::XMVECTOR g2 = dx::XMLoadFloat3(&g2_float);

		dx::XMVECTOR weight = dx::XMVectorSet(0.5, 0.5f, 0.5f, 0.0f);
		dx::XMVECTOR res = dx::XMVectorAdd(dx::XMVectorMultiply(weight, g1), dx::XMVectorMultiply(weight, g2));

		dx::XMFLOAT3 profi{};
		dx::XMStoreFloat3(&profi, res);

		return profi;//dx::XMFLOAT3(res.m128_f32[0], res.m128_f32[1], res.m128_f32[2]);
	}

	void separable_sss_calculate_kernel()
	{
		HRESULT hr;

		const float RANGE = num_samples > 20 ? 3.0f : 2.0f;
		const float EXPONENT = 2.0f;

		kernel.resize(num_samples);

		// Calculate the offsets:
		float step = 2.0f * RANGE / (num_samples - 1);
		for (int i = 0; i < num_samples; i++)
		{
			float o = -RANGE + float(i) * step;
			float sign = o < 0.0f ? -1.0f : 1.0f;
			kernel[i].w = RANGE * sign * abs(pow(o, EXPONENT)) / pow(RANGE, EXPONENT);
		}

		// Calculate the weights:
		for (int i = 0; i < num_samples; i++)
		{
			float w0 = i > 0 ? abs(kernel[i].w - kernel[i - 1].w) : 0.0f;
			float w1 = i < num_samples - 1 ? abs(kernel[i].w - kernel[i + 1].w) : 0.0f;
			float area = (w0 + w1) / 2.0f;
			dx::XMFLOAT3 prof = profile(kernel[i].w);
			dx::XMFLOAT3 t = dx::XMFLOAT3(area * prof.x, area * prof.y, area * prof.z);
			kernel[i].x = t.x;
			kernel[i].y = t.y;
			kernel[i].z = t.z;
		}

		// We want the offset 0.0 to come first:
		dx::XMFLOAT4 t = kernel[num_samples / 2];
		for (int i = num_samples / 2; i > 0; i--)
			kernel[i] = kernel[i - 1];
		kernel[0] = t;

		// Calculate the sum of the weights, we will need to normalize them below:
		dx::XMFLOAT3 sum = dx::XMFLOAT3(0.0f, 0.0f, 0.0f);
		for (int i = 0; i < num_samples; i++)
		{
			sum = dx::XMFLOAT3(kernel[i].x + sum.x, kernel[i].y + sum.y, kernel[i].z + sum.z);
		}

		// Normalize the weights:
		for (int i = 0; i < num_samples; i++)
		{
			kernel[i].x /= sum.x;
			kernel[i].y /= sum.y;
			kernel[i].z /= sum.z;
		}

		// Tweak them using the desired strength. The first one is:
		//     lerp(1.0, kernel[0].rgb, strength)
		//kernel[0].x = (1.0f - strength.x) * 1.0f + strength.x * kernel[0].x;
		//kernel[0].y = (1.0f - strength.y) * 1.0f + strength.y * kernel[0].y;
		//kernel[0].z = (1.0f - strength.z) * 1.0f + strength.z * kernel[0].z;

		//// The others:
		////     lerp(0.0, kernel[0].rgb, strength)
		//for (int i = 1; i < nSamples; i++) {
		//    kernel[i].x *= strength.x;
		//    kernel[i].y *= strength.y;
		//    kernel[i].z *= strength.z;
		//}

	}


	HRESULT Renderer_Backend::init(bool reset)
	{

		if (reset) rhi.m_device.release_all_resources();

		ZRY_Result zr;
		HRESULT hr;

		rhi.m_context->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)&m_annot);

		m_scene_viewport.TopLeftX = 0.0f;
		m_scene_viewport.TopLeftY = 0.0f;
		m_scene_viewport.Width = g_resolutionWidth;
		m_scene_viewport.Height = g_resolutionHeight;
		m_scene_viewport.MinDepth = 0.0f;
		m_scene_viewport.MaxDepth = 1.0f;

		m_shadow_map_viewport.TopLeftX = 0.0f;
		m_shadow_map_viewport.TopLeftY = 0.0f;
		m_shadow_map_viewport.Width = 2048.0f;
		m_shadow_map_viewport.Height = 2048.f;
		m_shadow_map_viewport.MinDepth = 0.0f;
		m_shadow_map_viewport.MaxDepth = 1.0f;

		D3D11_BUFFER_DESC material_cb_desc;
		ZeroMemory(&material_cb_desc, sizeof(material_cb_desc));
		material_cb_desc.ByteWidth = sizeof(Material_Params);
		material_cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		material_cb_desc.Usage = D3D11_USAGE_DEFAULT;

		zr = rhi.m_device.create_constant_buffer(&m_hnd_object_cb, "matPrms");
		RETURN_IF_FAILED(zr.value);
		//HRESULT hr = rhi.device.device->CreateBuffer(&matCbDesc, nullptr, &matPrmsCB);

		//RETURN_IF_FAILED(hr);

		//Some shader setup--------------------------------

		m_fullscreen_quad_shader = Vertex_Shader::create(VShader_ID::FULL_QUAD, nullptr, 0);

		m_sssss_pixel_shader = Pixel_Shader::create(PShader_ID::SSSSS);
		m_lighting_shader = Pixel_Shader::create(PShader_ID::LIGHTING);

		//World transform constant buffer setup---------------------------------------------------
		D3D11_BUFFER_DESC cb_obj_desc;
		cb_obj_desc.Usage = D3D11_USAGE_DEFAULT;
		cb_obj_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cb_obj_desc.CPUAccessFlags = 0;
		cb_obj_desc.MiscFlags = 0;
		cb_obj_desc.ByteWidth = sizeof(Obj_CB);

		hr = rhi.m_device.m_device->CreateBuffer(&cb_obj_desc, nullptr, &m_object_cb);
		RETURN_IF_FAILED(hr);

		rhi.m_context->VSSetConstantBuffers(0, 1, &m_object_cb);
		//---------------------------------------------------


		//View matrix constant buffer setup-------------------------------------------------------------
		D3D11_BUFFER_DESC cb_cam_desc;
		cb_cam_desc.Usage = D3D11_USAGE_DEFAULT;
		cb_cam_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cb_cam_desc.CPUAccessFlags = 0;
		cb_cam_desc.MiscFlags = 0;
		cb_cam_desc.ByteWidth = sizeof(View_CB);

		hr = rhi.m_device.m_device->CreateBuffer(&cb_cam_desc, nullptr, &m_view_cb);
		RETURN_IF_FAILED(hr);

		rhi.m_context->VSSetConstantBuffers(1, 1, &m_view_cb);
		//----------------------------------------------------------------


		//Projection matrix constant buffer setup--------------------------------------
		D3D11_BUFFER_DESC cb_proj_desc;
		cb_proj_desc.Usage = D3D11_USAGE_DEFAULT;
		cb_proj_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cb_proj_desc.CPUAccessFlags = 0;
		cb_proj_desc.MiscFlags = 0;
		cb_proj_desc.ByteWidth = sizeof(Proj_CB);

		hr = rhi.m_device.m_device->CreateBuffer(&cb_proj_desc, nullptr, &m_proj_cb);
		RETURN_IF_FAILED(hr);

		rhi.m_context->VSSetConstantBuffers(2, 1, &m_proj_cb);
		//------------------------------------------------------------------

		//Light constant buffer setup---------------------------------------
		D3D11_BUFFER_DESC light_buff_desc;
		ZeroMemory(&light_buff_desc, sizeof(light_buff_desc));
		light_buff_desc.Usage = D3D11_USAGE_DEFAULT;
		light_buff_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		light_buff_desc.ByteWidth = sizeof(Scene_Lights);


		zr = rhi.m_device.create_constant_buffer(&m_hnd_frame_cb, "light");
		//hr = rhi.device.device->CreateBuffer(&lightBuffDesc, nullptr, &lightsCB);
		RETURN_IF_FAILED(zr.value);

		//rhi.context->PSSetConstantBuffers(0, 1, &lightsCB);
		//rhi.context->VSSetConstantBuffers(3, 1, &lightsCB);
		//---------------------------------------------------------

		//GBuffer setup--------------------------------------------

		m_gbuffer_vertex_shader = Vertex_Shader::create(VShader_ID::STANDARD, s_vertex_layout_desc, ARRAYSIZE(s_vertex_layout_desc));

		Render_Texture_Handle hnd_gbuff[G_Buffer::SIZE];
		zr = rhi.m_device.create_tex_2d(&hnd_gbuff[0], ZRY_Bind_Flags{ D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE }, ZRY_Format{ DXGI_FORMAT_R8G8B8A8_TYPELESS }, g_resolutionWidth, g_resolutionHeight, G_Buffer::SIZE, nullptr, nullptr, true, 4, 1);
		RETURN_IF_FAILED(zr.value);
		for (int i = 0; i < G_Buffer::SIZE; i++)
		{
			m_gbuffer[i] = rhi.m_device.get_tex_2d_pointer(hnd_gbuff[i]);
		}

		Render_SRV_Handle hnd_gbuff_srv[G_Buffer::SIZE];
		zr = rhi.m_device.create_srv_tex_2d(hnd_gbuff_srv, hnd_gbuff, ZRY_Format{ DXGI_FORMAT_R8G8B8A8_UNORM }, G_Buffer::SIZE, 4);
		RETURN_IF_FAILED(zr.value);
		for (int i = 0; i < G_Buffer::SIZE; i++)
		{
			m_gbuffer_srv[i] = rhi.m_device.get_srv_pointer(hnd_gbuff_srv[i]);
		}

		Render_RTV_Handle hnd_gbuff_rtv[G_Buffer::SIZE];
		zr = rhi.m_device.create_rtv_tex_2d(hnd_gbuff_rtv, hnd_gbuff, ZRY_Format{ DXGI_FORMAT_R8G8B8A8_UNORM }, G_Buffer::SIZE);
		RETURN_IF_FAILED(zr.value);
		for (int i = 0; i < G_Buffer::SIZE; i++)
		{
			m_gbuffer_rtv[i] = rhi.m_device.get_rtv_pointer(hnd_gbuff_rtv[i]);
		}

		//----------------------------------------------------------

		//ambient setup------------------------------------------------
		Render_Texture_Handle hnd_ambient;
		zr = rhi.m_device.create_tex_2d(&hnd_ambient, ZRY_Bind_Flags{ D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE }, ZRY_Format{ DXGI_FORMAT_R8G8B8A8_TYPELESS }, g_resolutionWidth, g_resolutionHeight, 1, nullptr, nullptr, true, 0, 1);
		RETURN_IF_FAILED(zr.value);
		m_ambient_map = rhi.m_device.get_tex_2d_pointer(hnd_ambient);

		Render_SRV_Handle hnd_ambient_srv;
		zr = rhi.m_device.create_srv_tex_2d(&hnd_ambient_srv, &hnd_ambient, ZRY_Format{ DXGI_FORMAT_R8G8B8A8_UNORM });
		RETURN_IF_FAILED(zr.value);
		m_ambient_srv = rhi.m_device.get_srv_pointer(hnd_ambient_srv);

		Render_RTV_Handle hnd_ambient_rtv;
		zr = rhi.m_device.create_rtv_tex_2d(&hnd_ambient_rtv, &hnd_ambient, ZRY_Format{ DXGI_FORMAT_R8G8B8A8_UNORM }, 1);
		RETURN_IF_FAILED(zr.value);
		m_ambient_rtv = rhi.m_device.get_rtv_pointer(hnd_ambient_rtv);
		//-----------------------------------------------------------


		D3D11_BUFFER_DESC inv_matrix_cb_desc;
		ZeroMemory(&inv_matrix_cb_desc, sizeof(inv_matrix_cb_desc));
		inv_matrix_cb_desc.ByteWidth = sizeof(dx::XMMatrixIdentity()) * 2;
		inv_matrix_cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		inv_matrix_cb_desc.Usage = D3D11_USAGE_DEFAULT;

		hr = rhi.m_device.m_device->CreateBuffer(&inv_matrix_cb_desc, nullptr, &m_inv_mat_cb);

		//---------------------------------------------------------

		//Shadow map setup-------------------------------------------------
		m_shadow_map_vertex_shader = Vertex_Shader::create(VShader_ID::DEPTH, s_vertex_layout_desc, ARRAYSIZE(s_vertex_layout_desc));
		m_shadow_map_pixel_shader = Pixel_Shader::create(PShader_ID::SHADOW_MAP);

		D3D11_BUFFER_DESC dir_shadow_map_buff_desc;
		ZeroMemory(&dir_shadow_map_buff_desc, sizeof(D3D11_BUFFER_DESC));
		dir_shadow_map_buff_desc.Usage = D3D11_USAGE_DEFAULT;
		dir_shadow_map_buff_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		dir_shadow_map_buff_desc.ByteWidth = sizeof(Dir_Shadow_CB);

		hr = rhi.m_device.m_device->CreateBuffer(&dir_shadow_map_buff_desc, nullptr, &m_dir_shad_cb);
		RETURN_IF_FAILED(hr);

		D3D11_BUFFER_DESC omni_dir_shadow_map_buff_desc;
		ZeroMemory(&omni_dir_shadow_map_buff_desc, sizeof(D3D11_BUFFER_DESC));
		omni_dir_shadow_map_buff_desc.Usage = D3D11_USAGE_DEFAULT;
		omni_dir_shadow_map_buff_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		omni_dir_shadow_map_buff_desc.ByteWidth = sizeof(Omni_Dir_Shadow_CB);

		hr = rhi.m_device.m_device->CreateBuffer(&omni_dir_shadow_map_buff_desc, nullptr, &m_omni_dir_shad_cb);
		RETURN_IF_FAILED(hr);


		Render_Texture_Handle hnd_shadow_map;
		zr = rhi.m_device.create_tex_2d(&hnd_shadow_map, ZRY_Bind_Flags{ D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL }, ZRY_Format{ DXGI_FORMAT_R24G8_TYPELESS }, m_shadow_map_viewport.Width, m_shadow_map_viewport.Height, 1, nullptr, nullptr, false, 1);
		RETURN_IF_FAILED(zr.value);
		m_shadow_map = rhi.m_device.get_tex_2d_pointer(hnd_shadow_map);

		Render_SRV_Handle hnd_shadow_map_srv;
		zr = rhi.m_device.create_srv_tex_2d(&hnd_shadow_map_srv, &hnd_shadow_map, ZRY_Format{ DXGI_FORMAT_R24_UNORM_X8_TYPELESS }, 1, -1);
		RETURN_IF_FAILED(zr.value);
		m_shadow_map_srv = rhi.m_device.get_srv_pointer(hnd_shadow_map_srv);

		Render_DSV_Handle hnd_shadow_map_dsv;
		zr = rhi.m_device.create_dsv_tex_2d(&hnd_shadow_map_dsv, &hnd_shadow_map, ZRY_Format{ DXGI_FORMAT_D24_UNORM_S8_UINT });
		RETURN_IF_FAILED(zr.value);
		m_shadow_map_dsv = rhi.m_device.get_dsv_pointer(hnd_shadow_map_dsv);


		//Shadow map setup (spot light)-----------------------------------
		Render_Texture_Handle hnd_spot_shadow_map;
		zr = rhi.m_device.create_tex_2d(&hnd_spot_shadow_map, ZRY_Bind_Flags{ D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL }, ZRY_Format{ DXGI_FORMAT_R24G8_TYPELESS }, m_shadow_map_viewport.Width, m_shadow_map_viewport.Height, 1, nullptr, nullptr, false, 1);
		RETURN_IF_FAILED(zr.value);
		m_spot_shadow_map = rhi.m_device.get_tex_2d_pointer(hnd_spot_shadow_map);

		Render_SRV_Handle hnd_spot_shadow_map_srv;
		zr = rhi.m_device.create_srv_tex_2d(&hnd_spot_shadow_map_srv, &hnd_spot_shadow_map, ZRY_Format{ DXGI_FORMAT_R24_UNORM_X8_TYPELESS }, 1, -1);
		RETURN_IF_FAILED(zr.value);
		m_spot_shadow_map_srv = rhi.m_device.get_srv_pointer(hnd_spot_shadow_map_srv);

		Render_DSV_Handle hnd_spot_shadow_map_dsv;
		zr = rhi.m_device.create_dsv_tex_2d(&hnd_spot_shadow_map_dsv, &hnd_spot_shadow_map, ZRY_Format{ DXGI_FORMAT_D24_UNORM_S8_UINT });
		RETURN_IF_FAILED(zr.value);
		m_spot_shadow_map_desv = rhi.m_device.get_dsv_pointer(hnd_spot_shadow_map_dsv);


		//Cube shadow map setup (point light)--------------------------------------------
		Render_Texture_Handle hnd_shadow_cubemap;
		zr = rhi.m_device.createTexCubemap(&hnd_shadow_cubemap, ZRY_Bind_Flags{ D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL }, ZRY_Format{ DXGI_FORMAT_R24G8_TYPELESS },
			m_shadow_map_viewport.Width, m_shadow_map_viewport.Height, 1, nullptr, nullptr, false, 1, 12);
		RETURN_IF_FAILED(zr.value);
		m_shadow_cube_map = rhi.m_device.get_tex_2d_pointer(hnd_shadow_cubemap);

		Render_SRV_Handle hnd_shadow_cubemap_srv;
		zr = rhi.m_device.createSRVTex2DArray(&hnd_shadow_cubemap_srv, &hnd_shadow_cubemap, ZRY_Format{ DXGI_FORMAT_R24_UNORM_X8_TYPELESS }, 1, 12);
		RETURN_IF_FAILED(zr.value);
		m_shadow_cube_map_srv = rhi.m_device.get_srv_pointer(hnd_shadow_cubemap_srv);

		int num_shadow_cubemap_dsv = ARRAYSIZE(m_shadow_cube_map_dsv);
		Render_DSV_Handle* hnd_shadow_cubemap_dsv = new Render_DSV_Handle[num_shadow_cubemap_dsv];

		for (int i = 0; i < num_shadow_cubemap_dsv; i++)
		{
			zr = rhi.m_device.createDSVTex2DArray(&hnd_shadow_cubemap_dsv[i], &hnd_shadow_cubemap, ZRY_Format{ DXGI_FORMAT_D24_UNORM_S8_UINT }, 1, 1, 0, i);
			RETURN_IF_FAILED(zr.value);
			m_shadow_cube_map_dsv[i] = rhi.m_device.get_dsv_pointer(hnd_shadow_cubemap_dsv[i]);
		}

		//-----------------------------------------------------------------

		//Irradiance map setup---------------------------------------------

		Render_Texture_Handle hnd_skin_map[5];
		Render_RTV_Handle hnd_skin_rtv[5];
		Render_SRV_Handle hnd_skin_srv[5];
		zr = rhi.m_device.create_tex_2d(&hnd_skin_map[0], ZRY_Bind_Flags{ D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET }, ZRY_Format{ DXGI_FORMAT_R32G32B32A32_FLOAT }, g_resolutionWidth, g_resolutionHeight, 3, &hnd_skin_srv[0], &hnd_skin_rtv[0]);
		RETURN_IF_FAILED(zr.value);

		zr = rhi.m_device.create_tex_2d(&hnd_skin_map[4], ZRY_Bind_Flags{ D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET }, ZRY_Format{ DXGI_FORMAT_R32G32B32A32_FLOAT }, g_resolutionWidth, g_resolutionHeight, 1, &hnd_skin_srv[4], &hnd_skin_rtv[4]);
		RETURN_IF_FAILED(zr.value);

		zr = rhi.m_device.create_tex_2d(&hnd_skin_map[3], ZRY_Bind_Flags{ D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET }, ZRY_Format{ DXGI_FORMAT_R32G32B32A32_FLOAT }, g_resolutionWidth, g_resolutionHeight, 1, &hnd_skin_srv[3], &hnd_skin_rtv[3], true, 8);
		RETURN_IF_FAILED(zr.value);

		for (int i = 0; i < 5; i++)
		{
			m_skin_maps[i] = rhi.m_device.get_tex_2d_pointer(hnd_skin_map[i]);
			m_skin_rt[i] = rhi.m_device.get_rtv_pointer(hnd_skin_rtv[i]);
			m_skin_srv[i] = rhi.m_device.get_srv_pointer(hnd_skin_srv[i]);
		}

		//-----------------------------------------------------------------

		Render_Texture_Handle hnd_final_rt;
		zr = rhi.m_device.create_tex_2d(&hnd_final_rt, ZRY_Bind_Flags{ D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE }, ZRY_Format{ DXGI_FORMAT_R8G8B8A8_TYPELESS }, g_resolutionWidth, g_resolutionHeight, 1, nullptr, nullptr, false, 1, 1);
		RETURN_IF_FAILED(zr.value);
		m_final_render_target_tex = rhi.m_device.get_tex_2d_pointer(hnd_final_rt);

		Render_SRV_Handle hnd_final_srv;
		zr = rhi.m_device.create_srv_tex_2d(&hnd_final_srv, &hnd_final_rt, ZRY_Format{ DXGI_FORMAT_R8G8B8A8_UNORM });
		RETURN_IF_FAILED(zr.value);
		m_final_render_target_srv = rhi.m_device.get_srv_pointer(hnd_final_srv);

		Render_RTV_Handle hnd_final_rtv;
		zr = rhi.m_device.create_rtv_tex_2d(&hnd_final_rtv, &hnd_final_rt, ZRY_Format{ DXGI_FORMAT_R8G8B8A8_UNORM }, 1);
		RETURN_IF_FAILED(zr.value);
		m_final_render_target_view = rhi.m_device.get_rtv_pointer(hnd_final_rtv);

		//Depth stencil------------------------
		Render_Texture_Handle hnd_depth_tex;
		zr = rhi.m_device.create_tex_2d(&hnd_depth_tex, ZRY_Bind_Flags{ D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE }, ZRY_Format{ DXGI_FORMAT_R24G8_TYPELESS }, g_resolutionWidth, g_resolutionHeight, 1, nullptr, nullptr, false, 1, 1);
		RETURN_IF_FAILED(zr.value);
		m_depth_tex = rhi.m_device.get_tex_2d_pointer(hnd_depth_tex);

		Render_SRV_Handle hnd_depth_srv;
		zr = rhi.m_device.create_srv_tex_2d(&hnd_depth_srv, &hnd_depth_tex, ZRY_Format{ DXGI_FORMAT_R24_UNORM_X8_TYPELESS });
		RETURN_IF_FAILED(zr.value);
		m_depth_srv = rhi.m_device.get_srv_pointer(hnd_depth_srv);

		Render_DSV_Handle hnd_depth_dsv;
		zr = rhi.m_device.create_dsv_tex_2d(&hnd_depth_dsv, &hnd_depth_tex, ZRY_Format{ DXGI_FORMAT_D24_UNORM_S8_UINT }, 1);
		RETURN_IF_FAILED(zr.value);
		m_depth_dsv = rhi.m_device.get_dsv_pointer(hnd_depth_dsv);


		//----------------------------Skybox----------------------
		wrl::ComPtr<ID3D11Resource> sky_texture;
		hr = dx::CreateDDSTextureFromFileEx(rhi.m_device.m_device, L"./assets/skybox.dds", 0, D3D11_USAGE_DEFAULT,
			D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_TEXTURECUBE, dx::DX11::DDS_LOADER_DEFAULT, sky_texture.GetAddressOf(), &m_cubemap_view);
		RETURN_IF_FAILED(hr);

		m_skybox_vertex_shader = Vertex_Shader::create(VShader_ID::SKYBOX, s_vertex_layout_desc, ARRAYSIZE(s_vertex_layout_desc));
		m_skybox_pixel_shader = Pixel_Shader::create(PShader_ID::SKYBOX);

		//---------------------------------------------------------------------

		//thickness map
		//rhi.LoadTexture(L"./shaders/assets/Human/Textures/Head/JPG/baked_translucency_4096.jpg", thicknessMapSRV, false);

		std::vector<float> krn;
		////loadKernelFile("./shaders/assets/Skin2_PreInt_DISCSEP.bn", krn);
		load_kernel_file("./assets/Skin1_PreInt_DISCSEP.bn", krn);
		//loadKernelFile("./shaders/assets/Skin1_ArtModProd_DISCSEP.bn", krn);
		//
		override_ssss_discr_sep_kernel(krn);
		separable_sss_calculate_kernel();
		for (int i = 1; i < kernel.size(); i++)
		{
			OutputDebugString(std::to_string(kernel[i].w).c_str());
			OutputDebugString("\n");
		}

		return hr;
	}

	void Renderer_Backend::render_shadow_maps(const View_Desc& view_desc, Dir_Shadow_CB& dir_shadow_cb, Omni_Dir_Shadow_CB& omni_dir_shad_cb)
	{
		m_annot->BeginEvent(L"ShadowMap Pre-Pass");
		{
			std::vector<Light_Info> dir_lights;
			std::vector<Light_Info> spot_lights;
			std::vector<Light_Info> point_lights;

			View_CB tmp_view_cb;
			Proj_CB tmp_proj_cb;
			Obj_CB tmp_object_cb;
			std::uint32_t strides[] = { sizeof(Vertex) };
			std::uint32_t offsets[] = { 0 };


			rhi.m_context->RSSetViewports(1, &m_shadow_map_viewport);

			for (const Light_Info& light_info : view_desc.lights_info)
			{
				switch (light_info.tag)
				{
				case Light_Type::DIRECTIONAL:
				{
					dir_lights.push_back(light_info);
					break;
				}

				case Light_Type::SPOT:
				{
					spot_lights.push_back(light_info);
					break;
				}

				case Light_Type::POINT:
				{
					point_lights.push_back(light_info);
					break;
				}
				}
			}

			int num_spot_lights = spot_lights.size();
			assert(num_spot_lights == view_desc.num_spot_lights);

			int num_point_lights = point_lights.size();
			assert(num_point_lights == view_desc.num_point_lights);

			int num_dir_lights = dir_lights.size();
			assert(num_dir_lights == view_desc.num_dir_lights);

			rhi.m_context->PSSetShader(nullptr, nullptr, 0);

			rhi.m_context->VSSetShader(m_shadow_map_vertex_shader.m_shader, nullptr, 0);

			//Directional lights--------------------------------------------
			m_annot->BeginEvent(L"Directional Lights");

			rhi.m_context->ClearDepthStencilView(m_shadow_map_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
			rhi.m_context->OMSetRenderTargets(0, nullptr, m_shadow_map_dsv);

			m_shadow_map_vertex_shader.bind_constant_buffer("cbPerObj", m_object_cb);
			m_shadow_map_vertex_shader.bind_constant_buffer("cbPerCam", m_view_cb);
			m_shadow_map_vertex_shader.bind_constant_buffer("cbPerProj", m_proj_cb);
			//rhi.context->VSSetConstantBuffers(0, 1, &objectCB);
			//rhi.context->VSSetConstantBuffers(1, 1, &viewCB);
			//rhi.context->VSSetConstantBuffers(2, 1, &projCB);

			for (int i = 0; i < num_dir_lights; i++)
			{
				Directional_Light& dir_light = dir_lights.at(i).dir_light;
				dx::XMVECTOR transformed_dir_light = dx::XMVector4Transform(dx::XMLoadFloat4(&dir_light.dir), dir_lights.at(i).final_world_transform);
				dx::XMVECTOR dir_light_pos = dx::XMVectorMultiply(dx::XMVectorNegate(transformed_dir_light), dx::XMVectorSet(3.0f, 3.0f, 3.0f, 1.0f));
				//TODO: rename these matrices; it isnt clear that they are used for shadow mapping
				dx::XMMATRIX dir_light_view_matrix = dx::XMMatrixLookAtLH(dir_light_pos, dx::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
				dx::XMMATRIX dir_light_proj_matrix = dx::XMMatrixOrthographicLH(4.0f, 4.0f, dir_light.near_plane_dist, dir_light.far_plane_dist);

				dir_shadow_cb.dir_light_shadow_proj_mat = dx::XMMatrixTranspose(dir_light_proj_matrix);
				dir_shadow_cb.dir_light_shadow_view_mat = dx::XMMatrixTranspose(dir_light_view_matrix);
				omni_dir_shad_cb.dir_light_shadow_proj_mat = dir_shadow_cb.dir_light_shadow_proj_mat;
				omni_dir_shad_cb.dir_light_shadow_view_mat = dir_shadow_cb.dir_light_shadow_view_mat;

				tmp_view_cb.view_matrix = dx::XMMatrixTranspose(dir_light_view_matrix);
				tmp_proj_cb.proj_matrix = dx::XMMatrixTranspose(dir_light_proj_matrix);

				rhi.m_context->UpdateSubresource(m_view_cb, 0, nullptr, &tmp_view_cb, 0, 0);
				rhi.m_context->UpdateSubresource(m_proj_cb, 0, nullptr, &tmp_proj_cb, 0, 0);

				for (Submesh_Info const& submesh_info : view_desc.submeshes_info)
				{
					rhi.m_context->IASetVertexBuffers(0, 1, buffer_cache.get_vertex_buffer(submesh_info.hnd_buffer_cache).buffer.GetAddressOf(), strides, offsets);
					rhi.m_context->IASetIndexBuffer(buffer_cache.get_index_buffer(submesh_info.hnd_buffer_cache).buffer.Get(), DXGI_FORMAT_R16_UINT, 0);

					tmp_object_cb.world_matrix = dx::XMMatrixTranspose(submesh_info.final_world_transform);
					rhi.m_context->UpdateSubresource(m_object_cb, 0, nullptr, &tmp_object_cb, 0, 0);

					RHI_State state = RHI_DEFAULT_STATE();
					RHI_RS_SET_CULL_BACK(state);
					RHI_OM_DS_SET_DEPTH_COMP_LESS(state);

					rhi.set_state(state);

					rhi.m_context->DrawIndexed(submesh_info.hnd_submesh.num_indices, 0, 0);
				}
			}
			m_annot->EndEvent();

			//-------------------------------------------------------------------------------

			//Spot lights--------------------------------------------------------------------
			m_annot->BeginEvent(L"Spot Lights");

			rhi.m_context->OMSetRenderTargets(0, nullptr, m_spot_shadow_map_desv);
			rhi.m_context->ClearDepthStencilView(m_spot_shadow_map_desv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

			for (int i = 0; i < num_spot_lights; i++)
			{
				dx::XMVECTOR spot_light_final_pos = dx::XMVector4Transform(dx::XMLoadFloat4(&spot_lights[i].spot_light.pos_world_space), spot_lights[i].final_world_transform);
				dx::XMVECTOR spot_light_final_dir = dx::XMVector4Transform(dx::XMLoadFloat4(&spot_lights[i].spot_light.direction), spot_lights[i].final_world_transform);
				omni_dir_shad_cb.spot_light_proj_mat[i] = dx::XMMatrixTranspose(dx::XMMatrixPerspectiveFovLH(std::acos(spot_lights[i].spot_light.cos_cutoff_angle) * 2.0f, 1.0f, spot_lights[i].spot_light.near_plane_dist, spot_lights[i].spot_light.far_plane_dist));  //multiply acos by 2, because cutoff angle is considered from center, not entire light angle
				omni_dir_shad_cb.spot_light_view_mat[i] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(spot_light_final_pos, spot_light_final_dir, dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));

				tmp_view_cb.view_matrix = omni_dir_shad_cb.spot_light_view_mat[i];
				tmp_proj_cb.proj_matrix = omni_dir_shad_cb.spot_light_proj_mat[i];

				rhi.m_context->UpdateSubresource(m_view_cb, 0, nullptr, &tmp_view_cb, 0, 0);
				rhi.m_context->UpdateSubresource(m_proj_cb, 0, nullptr, &tmp_proj_cb, 0, 0);

				for (Submesh_Info const& submesh_info : view_desc.submeshes_info)
				{
					rhi.m_context->IASetVertexBuffers(0, 1, buffer_cache.get_vertex_buffer(submesh_info.hnd_buffer_cache).buffer.GetAddressOf(), strides, offsets);
					rhi.m_context->IASetIndexBuffer(buffer_cache.get_index_buffer(submesh_info.hnd_buffer_cache).buffer.Get(), DXGI_FORMAT_R16_UINT, 0);

					tmp_object_cb.world_matrix = dx::XMMatrixTranspose(submesh_info.final_world_transform);
					rhi.m_context->UpdateSubresource(m_object_cb, 0, nullptr, &tmp_object_cb, 0, 0);

					RHI_State state = RHI_DEFAULT_STATE();
					RHI_RS_SET_CULL_BACK(state);
					RHI_OM_DS_SET_DEPTH_COMP_LESS(state);

					rhi.set_state(state);

					rhi.m_context->DrawIndexed(submesh_info.hnd_submesh.num_indices, 0, 0);
				}
			}
			m_annot->EndEvent();

			//Point lights-------------------------------------------------------------------
			//TODO: do something about this; shouldnt hard code index in pointLights
			m_annot->BeginEvent(L"Point Lights");

			if (num_point_lights > 0) omni_dir_shad_cb.point_light_proj_mat = dx::XMMatrixTranspose(dx::XMMatrixPerspectiveFovLH(dx::XM_PIDIV2, 1.0f, point_lights[0].point_light.near_plane_dist, point_lights[0].point_light.far_plane_dist));

			for (int i = 0; i < num_point_lights; i++)
			{
				dx::XMVECTOR point_light_final_pos = dx::XMVector4Transform(dx::XMLoadFloat4(&point_lights[i].point_light.pos_world_space), point_lights[i].final_world_transform);
				omni_dir_shad_cb.point_light_view_mat[i * 6 + Cubemap::FACE_POSITIVE_X] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(point_light_final_pos, dx::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
				omni_dir_shad_cb.point_light_view_mat[i * 6 + Cubemap::FACE_NEGATIVE_X] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(point_light_final_pos, dx::XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
				omni_dir_shad_cb.point_light_view_mat[i * 6 + Cubemap::FACE_POSITIVE_Y] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(point_light_final_pos, dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f)));
				omni_dir_shad_cb.point_light_view_mat[i * 6 + Cubemap::FACE_NEGATIVE_Y] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(point_light_final_pos, dx::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f), dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f)));
				omni_dir_shad_cb.point_light_view_mat[i * 6 + Cubemap::FACE_POSITIVE_Z] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(point_light_final_pos, dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
				omni_dir_shad_cb.point_light_view_mat[i * 6 + Cubemap::FACE_NEGATIVE_Z] = dx::XMMatrixTranspose(dx::XMMatrixLookToLH(point_light_final_pos, dx::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f), dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));

				for (int face = 0; face < 6; face++)
				{
					rhi.m_context->ClearDepthStencilView(m_shadow_cube_map_dsv[i * 6 + face], D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
				}

				for (Submesh_Info const& submesh_info : view_desc.submeshes_info)
				{
					rhi.m_context->IASetVertexBuffers(0, 1, buffer_cache.get_vertex_buffer(submesh_info.hnd_buffer_cache).buffer.GetAddressOf(), strides, offsets);
					rhi.m_context->IASetIndexBuffer(buffer_cache.get_index_buffer(submesh_info.hnd_buffer_cache).buffer.Get(), DXGI_FORMAT_R16_UINT, 0);

					Obj_CB tmpOCB{ dx::XMMatrixTranspose(submesh_info.final_world_transform) };
					rhi.m_context->UpdateSubresource(m_object_cb, 0, nullptr, &tmpOCB, 0, 0);

					RHI_State state = RHI_DEFAULT_STATE();
					RHI_RS_SET_CULL_BACK(state);
					RHI_OM_DS_SET_DEPTH_COMP_LESS(state);

					rhi.set_state(state);

					tmp_proj_cb.proj_matrix = omni_dir_shad_cb.point_light_proj_mat;
					rhi.m_context->UpdateSubresource(m_proj_cb, 0, nullptr, &tmp_proj_cb, 0, 0);

					for (int face = 0; face < 6; face++)
					{
						tmp_view_cb.view_matrix = omni_dir_shad_cb.point_light_view_mat[i * 6 + face];
						rhi.m_context->UpdateSubresource(m_view_cb, 0, nullptr, &tmp_view_cb, 0, 0);

						rhi.m_context->OMSetRenderTargets(0, nullptr, m_shadow_cube_map_dsv[i * 6 + face]);
						rhi.m_context->DrawIndexed(submesh_info.hnd_submesh.num_indices, 0, 0);
					}

				}
			}
			m_annot->EndEvent();

		}
		m_annot->EndEvent();
	}


	void Renderer_Backend::render_view(const View_Desc& view_desc)
	{
		std::uint32_t strides[] = { sizeof(Vertex) };
		std::uint32_t offsets[] = { 0 };

		rb.m_annot->BeginEvent(L"Skybox Pass");
		{
			//Using cam rotation matrix as view, to ignore cam translation, making skybox always centered
			Obj_CB tmpOCB{ dx::XMMatrixIdentity() };
			View_CB tmpVCB{ dx::XMMatrixTranspose(view_desc.cam.get_rotation_matrix()) };
			Proj_CB tmpPCB{ view_desc.cam.get_proj_matrix_transposed() };

			rhi.m_context->VSSetShader(m_skybox_vertex_shader.m_shader, 0, 0);
			rhi.m_context->IASetInputLayout(nullptr);
			rhi.m_context->VSSetConstantBuffers(0, 1, &m_object_cb);
			rhi.m_context->VSSetConstantBuffers(1, 1, &m_view_cb);
			rhi.m_context->VSSetConstantBuffers(2, 1, &m_proj_cb);

			rhi.m_context->UpdateSubresource(m_object_cb, 0, nullptr, &tmpOCB, 0, 0);
			rhi.m_context->UpdateSubresource(m_view_cb, 0, nullptr, &tmpVCB, 0, 0);
			rhi.m_context->UpdateSubresource(m_proj_cb, 0, nullptr, &tmpPCB, 0, 0);

			rhi.m_context->PSSetShader(m_skybox_pixel_shader.m_shader, 0, 0);

			rhi.m_context->PSSetShaderResources(0, 1, &m_cubemap_view);

			RHI_OM_DS_SET_DEPTH_COMP_LESS_EQ(rhi_state);
			RHI_RS_SET_CULL_BACK(rhi_state);
			rhi.set_state(rhi_state);
			rhi.m_context->Draw(36, 0);
		}
		rb.m_annot->EndEvent();


		std::vector<Light_Info> dir_lights;
		std::vector<Light_Info> spot_lights;
		std::vector<Light_Info> point_lights;
		//TODO: remove this code here and in RenderShadowMaps; instead order the array of lights in buckets of same type 
		for (const Light_Info& light_info : view_desc.lights_info)
		{
			switch (light_info.tag)
			{
			case Light_Type::DIRECTIONAL:
			{
				dir_lights.push_back(light_info);
				break;
			}

			case Light_Type::SPOT:
			{
				spot_lights.push_back(light_info);
				break;
			}

			case Light_Type::POINT:
			{
				point_lights.push_back(light_info);
				break;
			}
			}
		}

		int num_spot_lights = spot_lights.size();
		assert(num_spot_lights == view_desc.num_spot_lights);

		int num_point_lights = point_lights.size();
		assert(num_point_lights == view_desc.num_point_lights);

		int num_dir_lights = dir_lights.size();
		assert(num_dir_lights == view_desc.num_dir_lights);

		rhi.m_context->VSSetConstantBuffers(0, 1, &m_object_cb);
		rhi.m_context->VSSetConstantBuffers(1, 1, &m_view_cb);
		rhi.m_context->VSSetConstantBuffers(2, 1, &m_proj_cb);

		rhi.m_context->IASetInputLayout(m_shadow_map_vertex_shader.m_vertex_input_layout);

		//Rendering shadow maps-------------------------------------------------
		Dir_Shadow_CB dir_shadow_cb{};
		Omni_Dir_Shadow_CB omni_dir_shadow_cb{};

		render_shadow_maps(view_desc, dir_shadow_cb, omni_dir_shadow_cb);
		//----------------------------------------------------------------------


		//Actual rendering-------------------------------------------------------
		rhi.m_context->RSSetViewports(1, &m_scene_viewport);

		View_CB tmpVCB{ view_desc.cam.get_view_matrix_transposed() };
		Proj_CB tmpPCB{ view_desc.cam.get_proj_matrix_transposed() };
		Scene_Lights tmp_lights_cb;

		for (int i = 0; i < num_dir_lights; i++)
		{
			Directional_Light& dir_light = dir_lights.at(i).dir_light;

			dx::XMVECTOR transformed_dir_light = dx::XMVector4Transform(dx::XMLoadFloat4(&dir_light.dir), dir_lights.at(i).final_world_transform);
			dx::XMStoreFloat4(&tmp_lights_cb.dir_light.dir, dx::XMVector4Transform(transformed_dir_light, view_desc.cam.get_view_matrix()));
			tmp_lights_cb.dir_light.near_plane_dist = dir_light.near_plane_dist;
			tmp_lights_cb.dir_light.far_plane_dist = dir_light.far_plane_dist;
		}

		for (int i = 0; i < num_point_lights; i++)
		{
			dx::XMVECTOR point_light_final_pos = dx::XMVector4Transform(dx::XMLoadFloat4(&point_lights[i].point_light.pos_world_space), point_lights[i].final_world_transform);
			tmp_lights_cb.point_pos_view_space[i] = dx::XMVector4Transform(point_light_final_pos, view_desc.cam.get_view_matrix());
			tmp_lights_cb.point_lights[i].constant = point_lights[i].point_light.constant;
			tmp_lights_cb.point_lights[i].lin = point_lights[i].point_light.lin;
			tmp_lights_cb.point_lights[i].quadratic = point_lights[i].point_light.quadratic;
			tmp_lights_cb.point_lights[i].near_plane_dist = point_lights[i].point_light.near_plane_dist;
			tmp_lights_cb.point_lights[i].far_plane_dist = point_lights[i].point_light.far_plane_dist;
		}
		tmp_lights_cb.num_point_lights = num_point_lights;

		for (int i = 0; i < view_desc.num_spot_lights; i++)
		{
			dx::XMVECTOR spot_light_final_pos = dx::XMVector4Transform(dx::XMLoadFloat4(&spot_lights[i].spot_light.pos_world_space), spot_lights[i].final_world_transform);
			dx::XMVECTOR spot_light_final_dir = dx::XMVector4Transform(dx::XMLoadFloat4(&spot_lights[i].spot_light.direction), spot_lights[i].final_world_transform);

			tmp_lights_cb.spot_lights[i].cos_cutoff_angle = spot_lights[i].spot_light.cos_cutoff_angle;
			dx::XMStoreFloat4(&tmp_lights_cb.spot_lights[i].pos_world_space, spot_light_final_pos);
			dx::XMStoreFloat4(&tmp_lights_cb.spot_lights[i].direction, spot_light_final_dir);

			tmp_lights_cb.spot_lights[i].near_plane_dist = spot_lights[i].spot_light.near_plane_dist;
			tmp_lights_cb.spot_lights[i].far_plane_dist = spot_lights[i].spot_light.far_plane_dist;

			tmp_lights_cb.spot_pos_view_space[i] = dx::XMVector4Transform(spot_light_final_pos, view_desc.cam.get_view_matrix());
			tmp_lights_cb.spot_dir_view_space[i] = dx::XMVector4Transform(spot_light_final_dir, view_desc.cam.get_view_matrix());
		}
		tmp_lights_cb.num_spot_ligthts = num_spot_lights;

		rhi.m_context->UpdateSubresource(m_view_cb, 0, nullptr, &tmpVCB, 0, 0);
		rhi.m_context->UpdateSubresource(m_proj_cb, 0, nullptr, &tmpPCB, 0, 0);
		//rhi.context->UpdateSubresource(lightsCB, 0, nullptr, &tmpLCB, 0, 0);
		rhi.m_device.update_constant_buffer(m_hnd_frame_cb, Frame_Constant_Buff{ tmp_lights_cb, Subsurface_Scattering_Params{} });
		rhi.m_context->UpdateSubresource(m_dir_shad_cb, 0, nullptr, &dir_shadow_cb, 0, 0);
		rhi.m_context->UpdateSubresource(m_omni_dir_shad_cb, 0, nullptr, &omni_dir_shadow_cb, 0, 0);

		m_annot->BeginEvent(L"G-Buffer Pass");
		{
			//TODO: temporary solution
			rhi.m_context->VSSetShader(m_gbuffer_vertex_shader.m_shader, nullptr, 0);
			rhi.m_context->IASetInputLayout(m_gbuffer_vertex_shader.m_vertex_input_layout);
			rhi.m_context->VSSetConstantBuffers(0, 1, &m_object_cb);
			rhi.m_context->VSSetConstantBuffers(1, 1, &m_view_cb);
			rhi.m_context->VSSetConstantBuffers(2, 1, &m_proj_cb);
			//GBufferVertexShader.bindConstantBuffer()
			//rhi.context->VSSetConstantBuffers(3, 1, &lightsCB);
			rhi.m_context->VSSetConstantBuffers(4, 1, &m_dir_shad_cb);
			rhi.m_context->VSSetConstantBuffers(5, 1, &m_omni_dir_shad_cb);

			FLOAT clear_color[4] = { 0.0f,0.0f,0.0f,1.0f };
			for (int i = 0; i < G_Buffer::SIZE; i++)
			{
				rhi.m_context->ClearRenderTargetView(m_gbuffer_rtv[i], clear_color);
			}

			ID3D11RenderTargetView* rt2[4] = { m_gbuffer_rtv[0], m_gbuffer_rtv[1], m_gbuffer_rtv[2], m_skin_rt[4] };
			rhi.m_context->OMSetRenderTargets(4, &rt2[0]/*&GBufferRTV[0]*/, m_depth_dsv);

			for (Submesh_Info const& submesh_info : view_desc.submeshes_info)
			{
				rhi.m_context->IASetVertexBuffers(0, 1, buffer_cache.get_vertex_buffer(submesh_info.hnd_buffer_cache).buffer.GetAddressOf(), strides, offsets);
				rhi.m_context->IASetIndexBuffer(buffer_cache.get_index_buffer(submesh_info.hnd_buffer_cache).buffer.Get(), DXGI_FORMAT_R16_UINT, 0);

				Material& mat = resource_cache.m_material_cache.at(submesh_info.hnd_material_cache.index);
				rhi.m_context->PSSetShader(mat.model.m_shader, 0, 0);
				mat.model.bind_texture_2d("ObjTexture", mat.albedo_map);
				mat.model.bind_texture_2d("NormalMap", mat.normal_map);
				mat.model.bind_texture_2d("MetalnessMap", mat.metalness_map);
				mat.model.bind_texture_2d("SmoothnessMap", mat.smoothness_map);

				mat.model.bind_texture_2d("ShadowMap", Shader_Texture2D{ m_shadow_map_srv });
				mat.model.bind_texture_2d("SpotShadowMap", Shader_Texture2D{ m_spot_shadow_map_srv });

				mat.model.bind_texture_2d("ThicknessMap", m_thickness_map_srv);

				rhi.m_context->PSSetShaderResources(5, 1, &m_shadow_cube_map_srv);

				rhi.m_device.update_constant_buffer(m_hnd_object_cb, Object_Constant_Buff{ mat.mat_prms });
				//rhi.context->UpdateSubresource(matPrmsCB, 0, nullptr, &mat.matPrms, 0, 0);
				//mat.model.bindConstantBuffer("")
				//rhi.context->PSSetConstantBuffers(0, 1, &lightsCB);

				Constant_Buffer& tmpObjBuff = rhi.m_device.m_constant_buffers.at(m_hnd_object_cb.index);
				mat.model.bind_constant_buffer("matPrms", tmpObjBuff.buffer);
				//rhi.context->PSSetConstantBuffers(1, 1, &matPrmsCB);
				rhi.m_context->PSSetConstantBuffers(2, 1, &m_omni_dir_shad_cb);

				Obj_CB tmpOCB{ dx::XMMatrixTranspose(submesh_info.final_world_transform) };
				rhi.m_context->UpdateSubresource(m_object_cb, 0, nullptr, &tmpOCB, 0, 0);

				RHI_State state = RHI_DEFAULT_STATE();
				RHI_RS_SET_CULL_BACK(state);
				RHI_OM_DS_SET_DEPTH_COMP_LESS(state);

				rhi.set_state(state);

				rhi.m_context->DrawIndexed(submesh_info.hnd_submesh.num_indices, 0, 0);
			}
		}
		m_annot->EndEvent();


		m_annot->BeginEvent(L"Lighting Pass");
		{
			ID3D11RenderTargetView* render_targets_light_pass[4] = { m_skin_rt[0], m_skin_rt[1], m_skin_rt[2], m_ambient_rtv };
			rhi.m_context->OMSetRenderTargets(4, &render_targets_light_pass[0], nullptr);

			rhi.m_context->VSSetShader(m_fullscreen_quad_shader.m_shader, nullptr, 0);
			rhi.m_context->IASetInputLayout(m_fullscreen_quad_shader.m_vertex_input_layout);
			rhi.m_context->IASetVertexBuffers(0, 0, NULL, strides, offsets);
			rhi.m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			rhi.m_context->PSSetShader(m_lighting_shader.m_shader, nullptr, 0);
			struct Im { dx::XMMATRIX invProj; dx::XMMATRIX invView; };
			Im im;
			im.invProj = dx::XMMatrixTranspose(dx::XMMatrixInverse(nullptr, view_desc.cam.get_proj_matrix()));
			im.invView = dx::XMMatrixTranspose(dx::XMMatrixInverse(nullptr, view_desc.cam.get_view_matrix()));
			rhi.m_context->UpdateSubresource(m_inv_mat_cb, 0, nullptr, &im, 0, 0);
			Constant_Buffer& constBuffTmp = rhi.m_device.m_constant_buffers.at(m_hnd_frame_cb.index);
			m_lighting_shader.bind_constant_buffer("light", constBuffTmp.buffer);
			//rhi.context->PSSetConstantBuffers(0, 1, &lightsCB);
			rhi.m_context->PSSetConstantBuffers(5, 1, &m_inv_mat_cb);
			rhi.m_context->PSSetShaderResources(0, 3, &m_gbuffer_srv[0]);
			rhi.m_context->PSSetShaderResources(3, 1, &m_depth_srv);
			rhi.m_context->PSSetShaderResources(4, 1, &m_shadow_map_srv);
			rhi.m_context->PSSetShaderResources(5, 1, &m_shadow_cube_map_srv);
			rhi.m_context->PSSetShaderResources(6, 1, &m_spot_shadow_map_srv);
			rhi.m_context->PSSetShaderResources(7, 1, &m_skin_srv[4]);
			rhi.m_context->Draw(4, 0);
		}
		m_annot->EndEvent();

		ID3D11ShaderResourceView* null_srv[8] = { nullptr };

		m_annot->BeginEvent(L"ShadowMap Pass");
		{
			//rhi.context->PSSetShaderResources(GBuffer::ALBEDO, 1, &nullSRV[0]);
			rhi.m_context->PSSetShaderResources(G_Buffer::ROUGH_MET, 1, &null_srv[0]);

			ID3D11RenderTargetView* render_targets_shadowmap_pass[2] = { m_skin_rt[3], m_gbuffer_rtv[G_Buffer::ROUGH_MET] };
			rhi.m_context->OMSetRenderTargets(2, &render_targets_shadowmap_pass[0], nullptr);

			rhi.m_context->PSSetShaderResources(0, 1, &m_skin_srv[0]);
			rhi.m_context->PSSetShaderResources(2, 1, &m_skin_srv[1]);
			rhi.m_context->PSSetShaderResources(7, 1, &m_ambient_srv);

			rhi.m_context->PSSetShader(m_shadow_map_pixel_shader.m_shader, nullptr, 0);
			Constant_Buffer& tmpConstBuf = rhi.m_device.m_constant_buffers.at(m_hnd_frame_cb.index);
			m_shadow_map_pixel_shader.bind_constant_buffer("light", tmpConstBuf.buffer);

			rhi.m_context->Draw(4, 0);
		}
		m_annot->EndEvent();

		m_annot->BeginEvent(L"SSSSS Pass");
		{

			if (view_desc.submeshes_info.size() > 0)
			{
				rhi.m_context->PSSetShaderResources(0, 3, null_srv);
				rhi.m_context->PSSetShader(m_sssss_pixel_shader.m_shader, nullptr, 0);

				Material& mat = resource_cache.m_material_cache.at(view_desc.submeshes_info.at(0).hnd_material_cache.index);
				Constant_Buffer& tmpFrameCB = rhi.m_device.m_constant_buffers.at(m_hnd_frame_cb.index);

				switch (mat.mat_prms.sss_model_id)
				{
				case ((std::uint32_t)SSS_MODEL::GOLUBEV):
				{
					//TODO: fix warning generated by shader requiring multiple render targets (for now
					// suppressed providing useless render target e.g. skinRT[0])
					ID3D11RenderTargetView* render_targets_golubev_sss_pass[] = { m_final_render_target_view, m_skin_rt[0] };

					rhi.m_context->OMSetRenderTargets(2, render_targets_golubev_sss_pass, nullptr);

					m_sssss_pixel_shader.bind_constant_buffer(tmpFrameCB.constant_buffer_name, tmpFrameCB.buffer);
					rhi.m_device.update_constant_buffer(m_hnd_frame_cb, Frame_Constant_Buff{ tmp_lights_cb, mat.sss_prms });

					rhi.m_context->PSSetShaderResources(0, 1, &m_skin_srv[3]);
					rhi.m_context->PSSetShaderResources(1, 1, &m_gbuffer_srv[G_Buffer::ROUGH_MET]);
					rhi.m_context->PSSetShaderResources(2, 1, &m_skin_srv[2]);
					rhi.m_context->PSSetShaderResources(3, 1, &m_depth_srv);
					rhi.m_context->PSSetShaderResources(4, 1, &m_gbuffer_srv[G_Buffer::ALBEDO]);
					rhi.m_context->Draw(4, 0);

					break;
				}

				case ((std::uint32_t)SSS_MODEL::JIMENEZ_SEPARABLE):
				{

					ID3D11RenderTargetView* render_targets_sep_sss_pass[] = { m_skin_rt[1], m_skin_rt[0] };

					m_annot->BeginEvent(L"Horizontal Pass");
					{
						rhi.m_context->PSSetShaderResources(0, 1, null_srv);
						rhi.m_context->OMSetRenderTargets(2, render_targets_sep_sss_pass, nullptr);

						m_sssss_pixel_shader.bind_constant_buffer(tmpFrameCB.constant_buffer_name, tmpFrameCB.buffer);
						mat.sss_prms.dir = dx::XMFLOAT2(1.0f, 0.0f);
						memcpy_s(mat.sss_prms.jimenez_samples_sss, sizeof(mat.sss_prms.jimenez_samples_sss), kernel.data(), kernel.size() * sizeof(dx::XMFLOAT4));
						rhi.m_device.update_constant_buffer(m_hnd_frame_cb, Frame_Constant_Buff{ tmp_lights_cb, mat.sss_prms });

						rhi.m_context->PSSetShaderResources(0, 1, &m_skin_srv[3]);
						rhi.m_context->PSSetShaderResources(1, 1, &m_gbuffer_srv[G_Buffer::ROUGH_MET]);
						rhi.m_context->PSSetShaderResources(2, 1, &m_skin_srv[2]);
						rhi.m_context->PSSetShaderResources(3, 1, &m_depth_srv);
						rhi.m_context->PSSetShaderResources(4, 1, &m_gbuffer_srv[G_Buffer::ALBEDO]);
						rhi.m_context->Draw(4, 0);
					}
					m_annot->EndEvent();

					render_targets_sep_sss_pass[0] = m_final_render_target_view;

					m_annot->BeginEvent(L"Vertical Pass");
					{
						mat.sss_prms.dir = dx::XMFLOAT2(0.0f, 1.0f);
						rhi.m_device.update_constant_buffer(m_hnd_frame_cb, Frame_Constant_Buff{ tmp_lights_cb, mat.sss_prms });
						rhi.m_context->OMSetRenderTargets(2, render_targets_sep_sss_pass, nullptr);
						rhi.m_context->PSSetShaderResources(0, 1, &m_skin_srv[1]);
						rhi.m_context->Draw(4, 0);
					}
					m_annot->EndEvent();

					break;
				}

				case ((std::uint32_t)SSS_MODEL::JIMENEZ_GAUSS):
				{

					ID3D11RenderTargetView* render_targets_gauss_sss_pass[4] = {
						m_skin_rt[0],
						m_skin_rt[1],

						m_final_render_target_view,
						m_skin_rt[3]
					};


					float blend_factors[4][4] = {
						{1.0f, 1.0f, 1.0f, 1.0f},
						{0.3251f, 0.45f, 0.3583f, 1.0f},
						{0.34f, 0.1864f, 0.0f, 1.0f},
						{0.46f, 0.0f, 0.0402f, 1.0f}
					};

					float gauss_standard_deviations[4] = { 0.08f, 0.2126f, 0.4694f, 1.3169f };

					memcpy_s(mat.sss_prms.jimenez_samples_sss, sizeof(mat.sss_prms.jimenez_samples_sss), kernel.data(), kernel.size() * sizeof(dx::XMFLOAT4));

					for (int i = 0; i < 4; i++)
					{
						mat.sss_prms.mean_free_path_dist = gauss_standard_deviations[i];

						m_annot->BeginEvent(L"Horizontal Pass");
						{
							rhi.m_context->PSSetShaderResources(0, 1, null_srv);
							rhi.m_context->OMSetRenderTargets(2, &render_targets_gauss_sss_pass[0], nullptr);

							m_sssss_pixel_shader.bind_constant_buffer(tmpFrameCB.constant_buffer_name, tmpFrameCB.buffer);
							mat.sss_prms.dir = dx::XMFLOAT2(1.0f, 0.0f);
							rhi.m_device.update_constant_buffer(m_hnd_frame_cb, Frame_Constant_Buff{ tmp_lights_cb, mat.sss_prms });

							rhi.m_context->OMSetBlendState(NULL, 0, D3D11_DEFAULT_SAMPLE_MASK);

							rhi.m_context->PSSetShaderResources(0, 1, &m_skin_srv[3]);
							rhi.m_context->PSSetShaderResources(1, 1, &m_gbuffer_srv[G_Buffer::ROUGH_MET]);
							rhi.m_context->PSSetShaderResources(2, 1, &m_skin_srv[2]);
							rhi.m_context->PSSetShaderResources(3, 1, &m_depth_srv);
							rhi.m_context->PSSetShaderResources(4, 1, &m_gbuffer_srv[G_Buffer::ALBEDO]);
							rhi.m_context->Draw(4, 0);
						}
						m_annot->EndEvent();


						m_annot->BeginEvent(L"Vertical Pass");
						{

							rhi.m_context->PSSetShaderResources(0, 1, null_srv);
							rhi.m_context->OMSetRenderTargets(2, &render_targets_gauss_sss_pass[2], nullptr);

							mat.sss_prms.dir = dx::XMFLOAT2(0.0f, 1.0f);
							rhi.m_device.update_constant_buffer(m_hnd_frame_cb, Frame_Constant_Buff{ tmp_lights_cb, mat.sss_prms });

							rhi.m_context->OMSetBlendState(rhi.get_blend_state(1), blend_factors[i], D3D11_DEFAULT_SAMPLE_MASK);

							rhi.m_context->PSSetShaderResources(0, 1, &m_skin_srv[1]);
							rhi.m_context->Draw(4, 0);
						}
						m_annot->EndEvent();

					}

					rhi.m_context->OMSetBlendState(NULL, 0, D3D11_DEFAULT_SAMPLE_MASK);

					break;
				}

				default:
				{
					rhi.m_context->PSSetShaderResources(0, 1, null_srv);
					ID3D11RenderTargetView* passRenderTargetViews[] = { m_final_render_target_view, m_skin_rt[0] };

					rhi.m_context->OMSetRenderTargets(2, passRenderTargetViews, nullptr);
					Constant_Buffer& tmpFrameCB = rhi.m_device.m_constant_buffers.at(m_hnd_frame_cb.index);
					m_sssss_pixel_shader.bind_constant_buffer(tmpFrameCB.constant_buffer_name, tmpFrameCB.buffer);
					rhi.m_device.update_constant_buffer(m_hnd_frame_cb, Frame_Constant_Buff{ tmp_lights_cb, mat.sss_prms });

					rhi.m_context->PSSetShaderResources(0, 1, &m_skin_srv[3]);
					rhi.m_context->PSSetShaderResources(1, 1, &m_gbuffer_srv[G_Buffer::ROUGH_MET]);
					rhi.m_context->PSSetShaderResources(2, 1, &m_skin_srv[2]);
					rhi.m_context->PSSetShaderResources(3, 1, &m_depth_srv);
					rhi.m_context->PSSetShaderResources(4, 1, &m_gbuffer_srv[G_Buffer::ALBEDO]);
					rhi.m_context->Draw(4, 0);

					break;
				}

				}


				//annot->BeginEvent(L"mips");
				//{
				//    rhi.context->GenerateMips(skinSRV[3]);
				//}
				//annot->EndEvent();

			}

		}
		m_annot->EndEvent();

		//rhi.context->OMSetRenderTargets(1, rhi.renderTargetView.GetAddressOf(), nullptr);
		//rhi.context->PSSetShader(shaders.pixelShaders.at((std::uint8_t)PShaderID::PRESENT), nullptr, 0);
		//rhi.context->PSSetShaderResources(0, 1, &GBufferSRV[GBuffer::ALBEDO]);
		//rhi.context->Draw(4, 0);


		rhi.m_context->PSSetShaderResources(0, 8, &null_srv[0]);

		rhi.m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

}