#include "renderer/frontend/Shaders.h"
#include "renderer/frontend/Shader.h"

#include "renderer/backend/rhi/RenderHardwareInterface.h"

#include <string>
#include <Windows.h>
#include <d3d11_1.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include <d3dcompiler.inl>

#include "shaders/headers/DepthPS.h"
#include "shaders/headers/DepthVS.h"
#include "shaders/headers/GBufferPS.h"
#include "shaders/headers/GBufferVS.h"
#include "shaders/headers/Lighting.h"
#include "shaders/headers/Present.h"
#include "shaders/headers/ShadowMapping.h"
#include "shaders/headers/SkyboxPS.h"
#include "shaders/headers/SkyboxVS.h"
#include "shaders/headers/SSSSS_NOOPTIONS.h"
#include "shaders/headers/FullscreenQuad.h"

//Shaders shaders;


namespace zorya
{
	Shaders::~Shaders()
	{}

	HRESULT Shaders::init()
	{
		m_vertex_shaders.resize((std::uint8_t)VShader_ID::NUM_SHADERS);
		m_pixel_shaders.resize((std::uint8_t)PShader_ID::NUM_SHADERS);

		m_vertex_layout = nullptr;
		m_simple_vertex_layout = nullptr;

		HRESULT hr = load_default_shaders();
		//BuildDefaultShaders();

		return hr;
	}

	HRESULT Shaders::build_default_shaders()
	{
		ID3DBlob* shader_blob = nullptr;
		HRESULT h_res;

		h_res = load_shader<ID3D11VertexShader>(L"./shaders/GBufferVS.hlsl", "vs", &shader_blob, &m_vertex_shaders.at((std::uint8_t)VShader_ID::STANDARD), rhi.m_device.m_device);
		RETURN_IF_FAILED(h_res);
		h_res = rhi.m_device.m_device->CreateInputLayout(s_vertex_layout_desc, 4, shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), &m_vertex_layout);
		RETURN_IF_FAILED(h_res);

		h_res = load_shader<ID3D11VertexShader>(L"./shaders/DepthVS.hlsl", "vs", &shader_blob, &m_vertex_shaders.at((std::uint8_t)VShader_ID::DEPTH), rhi.m_device.m_device);
		RETURN_IF_FAILED(h_res);
		h_res = load_shader<ID3D11VertexShader>(L"./shaders/SkyboxVS.hlsl", "vs", &shader_blob, &m_vertex_shaders.at((std::uint8_t)VShader_ID::SKYBOX), rhi.m_device.m_device);
		RETURN_IF_FAILED(h_res);
		h_res = load_shader<ID3D11VertexShader>(L"./shaders/FullcreenQuad.hlsl", "vs", &shader_blob, &m_vertex_shaders.at((std::uint8_t)VShader_ID::FULL_QUAD), rhi.m_device.m_device);
		RETURN_IF_FAILED(h_res);


		h_res = load_shader<ID3D11PixelShader>(L"./shaders/GBufferPS.hlsl", "ps", &shader_blob, &m_pixel_shaders.at((std::uint8_t)PShader_ID::STANDARD), rhi.m_device.m_device);
		RETURN_IF_FAILED(h_res);
		h_res = load_shader<ID3D11PixelShader>(L"./shaders/SkyboxPS.hlsl", "ps", &shader_blob, &m_pixel_shaders.at((std::uint8_t)PShader_ID::SKYBOX), rhi.m_device.m_device);
		RETURN_IF_FAILED(h_res);
		h_res = load_shader<ID3D11PixelShader>(L"./shaders/SSSSS.hlsl", "ps", &shader_blob, &m_pixel_shaders.at((std::uint8_t)PShader_ID::SSSSS), rhi.m_device.m_device);
		RETURN_IF_FAILED(h_res);
		h_res = load_shader<ID3D11PixelShader>(L"./shaders/Lighting.hlsl", "ps", &shader_blob, &m_pixel_shaders.at((std::uint8_t)PShader_ID::LIGHTING), rhi.m_device.m_device);
		RETURN_IF_FAILED(h_res);
		h_res = load_shader<ID3D11PixelShader>(L"./shaders/ShadowMapping.hlsl", "ps", &shader_blob, &m_pixel_shaders.at((std::uint8_t)PShader_ID::SHADOW_MAP), rhi.m_device.m_device);
		RETURN_IF_FAILED(h_res);
		h_res = load_shader<ID3D11PixelShader>(L"./shaders/Present.hlsl", "ps", &shader_blob, &m_pixel_shaders.at((std::uint8_t)PShader_ID::PRESENT), rhi.m_device.m_device);
		RETURN_IF_FAILED(h_res);

		if (shader_blob != nullptr) shader_blob->Release();

		return h_res;
	}

	HRESULT Shaders::load_default_shaders()
	{
		HRESULT h_res;

		h_res = rhi.m_device.m_device->CreateVertexShader(g_GBufferVS, sizeof(g_GBufferVS), nullptr, &m_vertex_shaders.at((std::uint8_t)VShader_ID::STANDARD));
		RETURN_IF_FAILED(h_res);
		h_res = rhi.m_device.m_device->CreateInputLayout(s_vertex_layout_desc, 4, g_GBufferVS, sizeof(g_GBufferVS), &m_vertex_layout);
		RETURN_IF_FAILED(h_res);

		h_res = rhi.m_device.m_device->CreateVertexShader(g_DepthVS, sizeof(g_DepthVS), nullptr, &m_vertex_shaders.at((std::uint8_t)VShader_ID::DEPTH));
		RETURN_IF_FAILED(h_res);
		h_res = rhi.m_device.m_device->CreateVertexShader(g_SkyboxVS, sizeof(g_SkyboxVS), nullptr, &m_vertex_shaders.at((std::uint8_t)VShader_ID::SKYBOX));
		RETURN_IF_FAILED(h_res);
		h_res = rhi.m_device.m_device->CreateVertexShader(g_FullscreenQuad, sizeof(g_FullscreenQuad), nullptr, &m_vertex_shaders.at((std::uint8_t)VShader_ID::FULL_QUAD));
		RETURN_IF_FAILED(h_res);

		h_res = rhi.m_device.m_device->CreatePixelShader(g_GBufferPS, sizeof(g_GBufferPS), nullptr, &m_pixel_shaders.at((std::uint8_t)PShader_ID::STANDARD));
		RETURN_IF_FAILED(h_res);
		h_res = rhi.m_device.m_device->CreatePixelShader(g_SkyboxPS, sizeof(g_SkyboxPS), nullptr, &m_pixel_shaders.at((std::uint8_t)PShader_ID::SKYBOX));
		RETURN_IF_FAILED(h_res);
		h_res = rhi.m_device.m_device->CreatePixelShader(g_SSSSS, sizeof(g_SSSSS), nullptr, &m_pixel_shaders.at((std::uint8_t)PShader_ID::SSSSS));
		RETURN_IF_FAILED(h_res);
		h_res = rhi.m_device.m_device->CreatePixelShader(g_Lighting, sizeof(g_Lighting), nullptr, &m_pixel_shaders.at((std::uint8_t)PShader_ID::LIGHTING));
		RETURN_IF_FAILED(h_res);
		h_res = rhi.m_device.m_device->CreatePixelShader(g_ShadowMapping, sizeof(g_ShadowMapping), nullptr, &m_pixel_shaders.at((std::uint8_t)PShader_ID::SHADOW_MAP));
		RETURN_IF_FAILED(h_res);
		h_res = rhi.m_device.m_device->CreatePixelShader(g_Present, sizeof(g_Present), nullptr, &m_pixel_shaders.at((std::uint8_t)PShader_ID::PRESENT));
		RETURN_IF_FAILED(h_res);


		//ID3D11ShaderReflection* shaderReflection = nullptr;
		//hRes = D3DReflect(g_SSSSS, sizeof(g_SSSSS), IID_ID3D11ShaderReflection, (void**)&shaderReflection);
		//RETURN_IF_FAILED(hRes);
		//D3D11_SHADER_DESC shaderDesc;
		//shaderReflection->GetDesc(&shaderDesc);

		return h_res;
	}

	void Shaders::release_all_resources()
	{
		for (ID3D11PixelShader* ps : m_pixel_shaders)
		{
			if (ps) ps->Release();
		}

		for (ID3D11VertexShader* vs : m_vertex_shaders)
		{
			if (vs) vs->Release();
		}

		if (m_vertex_layout) m_vertex_layout->Release();
	}

	template<>
	HRESULT create_shader<ID3D11VertexShader>(ID3DBlob* p_shader_blob, ID3D11VertexShader** p_shader, ID3D11Device* d3d_device)
	{
		return d3d_device->CreateVertexShader(p_shader_blob->GetBufferPointer(), p_shader_blob->GetBufferSize(), nullptr, p_shader);
	};

	template<>
	HRESULT create_shader<ID3D11PixelShader>(ID3DBlob* p_shader_blob, ID3D11PixelShader** p_shader, ID3D11Device* d3d_device)
	{
		return d3d_device->CreatePixelShader(p_shader_blob->GetBufferPointer(), p_shader_blob->GetBufferSize(), nullptr, p_shader);
	};

	template<>
	std::string get_latest_profile<ID3D11VertexShader>()
	{
		return "vs_5_0";
	}

	template<>
	std::string get_latest_profile<ID3D11PixelShader>()
	{
		return "ps_5_0";
	}
}