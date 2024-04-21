#include "Shader.h"
#include "renderer/backend/rhi/RenderHardwareInterface.h"

//#include "shaders/headers/DepthPS.h"
//#include "shaders/headers/DepthVS.h"
//#include "shaders/headers/GBufferPS.h"
//#include "shaders/headers/GBufferVS.h"
//#include "shaders/headers/Lighting.h"
//#include "shaders/headers/Present.h"
//#include "shaders/headers/ShadowMapping.h"
//#include "shaders/headers/SkyboxPS.h"
//#include "shaders/headers/SkyboxVS.h"
//#include "shaders/headers/SSSSS.h"
//#include "shaders/headers/FullscreenQuad.h"

#include <cstdio>

namespace zorya
{
	//TODO: implement also shader unload from memory
	constexpr Shader_Bytecode load_shader_bytecode(const char* filepath)
	{
		FILE* file = nullptr;
		BYTE* bytecode = nullptr;
		size_t size_in_bytes = 0;

		if (fopen_s(&file, filepath, "rb") == 0)
		{
			fseek(file, 0, SEEK_END);
			size_in_bytes = ftell(file);
			fseek(file, 0, SEEK_SET);

			bytecode = (BYTE*)malloc(size_in_bytes);

			fread_s(bytecode, size_in_bytes, sizeof(BYTE), size_in_bytes / sizeof(BYTE), file);
			fclose(file);
		}

		return Shader_Bytecode{ bytecode, size_in_bytes };
	}

	ID3D11PixelShader* Pixel_Shader::s_registered_pixel_shaders[] = { nullptr };
	Shader_Bytecode Pixel_Shader::s_pixel_shader_bytecode_buffers[] = {
		Shader_Bytecode{},
		load_shader_bytecode("./shaders/GBufferPS.cso"), //ShaderBytecode{ g_GBufferPS, sizeof(g_GBufferPS) },
		load_shader_bytecode("./shaders/SkyboxPS.cso"), //ShaderBytecode{ g_SkyboxPS, sizeof(g_SkyboxPS) },
		Shader_Bytecode{},
		load_shader_bytecode("./shaders/SSSSS.cso"), //ShaderBytecode{ g_SSSSS, sizeof(g_SSSSS) },
		load_shader_bytecode("./shaders/Lighting.cso"), //ShaderBytecode{ g_Lighting, sizeof(g_Lighting) },
		load_shader_bytecode("./shaders/ShadowMapping.cso"), //ShaderBytecode{ g_ShadowMapping, sizeof(g_ShadowMapping) },
		load_shader_bytecode("./shaders/Present.cso"), //ShaderBytecode{ g_Present, sizeof(g_Present) }
	};

	Pixel_Shader Pixel_Shader::create(const BYTE* shader_bytecode, size_t bytecode_size)
	{
		ID3D11PixelShader* shader = nullptr;
		HRESULT h_res = rhi.m_device.m_device->CreatePixelShader(shader_bytecode, bytecode_size, nullptr, &shader);

		ID3D11ShaderReflection* shader_reflection = nullptr;
		h_res = D3DReflect(shader_bytecode, bytecode_size, IID_ID3D11ShaderReflection, (void**)&shader_reflection);

		return Pixel_Shader{ shader, shader_reflection };
	}

	Pixel_Shader Pixel_Shader::create(PShader_ID pixel_shader_id)
	{
		HRESULT h_res = S_OK;

		Shader_Bytecode shader_bytecode = s_pixel_shader_bytecode_buffers[(std::uint8_t)pixel_shader_id];
		ID3D11PixelShader* shader = s_registered_pixel_shaders[(std::uint8_t)pixel_shader_id];

		if (shader == nullptr)
		{
			h_res = rhi.m_device.m_device->CreatePixelShader(shader_bytecode.bytecode, shader_bytecode.size_in_bytes, nullptr, &shader);
			if (!FAILED(h_res)) s_registered_pixel_shaders[(std::uint8_t)pixel_shader_id] = shader;
		}
		else
		{
			shader->AddRef();
		}

		ID3D11ShaderReflection* shader_reflection = nullptr;
		h_res = D3DReflect(shader_bytecode.bytecode, shader_bytecode.size_in_bytes, IID_ID3D11ShaderReflection, (void**)&shader_reflection);

		return Pixel_Shader{ shader, shader_reflection };
	}

	void Pixel_Shader::free_shader()
	{
		if (m_shader)m_shader->Release();
		if (m_shader_reflection) m_shader_reflection->Release();
		//TODO: IMPORTANT! Add removal from registered shaders (in shader struct add its id, to know where to remove)
	}

	HRESULT Pixel_Shader::bind_texture_2d(const char* binding_name, const Shader_Texture2D& texture)
	{
		D3D11_SHADER_INPUT_BIND_DESC bind_desc;
		HRESULT hr = m_shader_reflection->GetResourceBindingDescByName(binding_name, &bind_desc);
		RETURN_IF_FAILED(hr);

		rhi.m_context->PSSetShaderResources(bind_desc.BindPoint, 1, &texture.resource_view);

		return S_OK;
	}

	HRESULT Pixel_Shader::bind_constant_buffer(const char* binding_name, ID3D11Buffer* const constant_buffer)
	{
		D3D11_SHADER_INPUT_BIND_DESC bind_desc;
		HRESULT hr = m_shader_reflection->GetResourceBindingDescByName(binding_name, &bind_desc);
		RETURN_IF_FAILED(hr);

		rhi.m_context->PSSetConstantBuffers(bind_desc.BindPoint, 1, &constant_buffer);

		return hr;
	}





	Vertex_Shader Vertex_Shader::create(const BYTE* shader_bytecode, size_t bytecode_size, D3D11_INPUT_ELEMENT_DESC* vertex_layout_desc, int num_input_elements)
	{
		ID3D11VertexShader* shader = nullptr;
		HRESULT hRes = rhi.m_device.m_device->CreateVertexShader(shader_bytecode, bytecode_size, nullptr, &shader);

		ID3D11InputLayout* vertex_input_layout = nullptr;
		if (vertex_layout_desc != nullptr)
		{
			hRes = rhi.m_device.m_device->CreateInputLayout(vertex_layout_desc, num_input_elements, shader_bytecode, bytecode_size, &vertex_input_layout);
		}

		ID3D11ShaderReflection* shader_reflection = nullptr;
		hRes = D3DReflect(shader_bytecode, bytecode_size, IID_ID3D11ShaderReflection, (void**)&shader_reflection);

		return Vertex_Shader{ shader, vertex_input_layout, shader_reflection };
	}

	Vertex_Shader Vertex_Shader::create(VShader_ID vertex_shader_id, D3D11_INPUT_ELEMENT_DESC* vertex_layout_desc, int num_input_elements)
	{
		HRESULT hRes = S_OK;

		Shader_Bytecode shader_bytecode = s_vertex_shader_bytecode_buffers[(std::uint8_t)vertex_shader_id];
		ID3D11VertexShader* shader = s_registered_vertex_shaders[(std::uint8_t)vertex_shader_id];

		if (shader == nullptr)
		{
			hRes = rhi.m_device.m_device->CreateVertexShader(shader_bytecode.bytecode, shader_bytecode.size_in_bytes, nullptr, &shader);
			if (!FAILED(hRes)) s_registered_vertex_shaders[(std::uint8_t)vertex_shader_id] = shader;
		}
		else
		{
			shader->AddRef();
		}

		ID3D11InputLayout* vertex_input_layout = nullptr;
		if (vertex_layout_desc != nullptr)
		{
			hRes = rhi.m_device.m_device->CreateInputLayout(vertex_layout_desc, num_input_elements, shader_bytecode.bytecode, shader_bytecode.size_in_bytes, &vertex_input_layout);
		}

		ID3D11ShaderReflection* shader_reflection = nullptr;
		hRes = D3DReflect(shader_bytecode.bytecode, shader_bytecode.size_in_bytes, IID_ID3D11ShaderReflection, (void**)&shader_reflection);

		return Vertex_Shader{ shader, vertex_input_layout, shader_reflection };
	}

	void Vertex_Shader::free_shader()
	{
		if (m_shader)m_shader->Release();
		if (m_shader_reflection) m_shader_reflection->Release();
		if (m_vertex_input_layout) m_vertex_input_layout->Release();
		//TODO: IMPORTANT! Add removal from registered shaders (in shader struct add its id, to know where to remove)
	}

	HRESULT Vertex_Shader::bind_constant_buffer(const char* binding_name, ID3D11Buffer* const constant_buffer)
	{
		D3D11_SHADER_INPUT_BIND_DESC bind_desc;
		HRESULT hr = m_shader_reflection->GetResourceBindingDescByName(binding_name, &bind_desc);
		RETURN_IF_FAILED(hr);

		rhi.m_context->PSSetConstantBuffers(bind_desc.BindPoint, 1, &constant_buffer);

		return hr;
	}

	ID3D11VertexShader* Vertex_Shader::s_registered_vertex_shaders[] = { nullptr };
	Shader_Bytecode Vertex_Shader::s_vertex_shader_bytecode_buffers[] = {
		load_shader_bytecode("./shaders/GBufferVS.cso"), //ShaderBytecode{ g_GBufferVS, sizeof(g_GBufferVS) },
		load_shader_bytecode("./shaders/SkyboxVS.cso"), //ShaderBytecode{ g_SkyboxVS, sizeof(g_SkyboxVS) },
		load_shader_bytecode("./shaders/DepthVS.cso"), //ShaderBytecode{ g_DepthVS, sizeof(g_DepthVS) },
		load_shader_bytecode("./shaders/FullscreenQuad.cso"), //ShaderBytecode{ g_FullscreenQuad, sizeof(g_FullscreenQuad) }
	};
}