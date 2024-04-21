#ifndef ZORYA_SHADER_H_
#define ZORYA_SHADER_H_

#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <cstdint>

namespace zorya
{
	enum class VShader_ID : uint8_t
	{
		STANDARD,
		SKYBOX,
		DEPTH,
		FULL_QUAD,
		NUM_SHADERS
	};

	enum class PShader_ID : uint8_t
	{
		UNDEFINED,
		STANDARD,
		SKYBOX,
		SKIN,
		SSSSS,
		LIGHTING,
		SHADOW_MAP,
		PRESENT,
		NUM_SHADERS
	};


	static D3D11_INPUT_ELEMENT_DESC s_vertex_layout_desc[4] = {
		{"position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "texcoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "tangent", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};


	struct Shader_Bytecode
	{
		const BYTE* bytecode;
		size_t size_in_bytes;
	};

	struct Shader_Texture2D
	{
		ID3D11ShaderResourceView* resource_view;
	};

	struct Pixel_Shader
	{
		ID3D11PixelShader* m_shader;
		ID3D11ShaderReflection* m_shader_reflection;

		static Pixel_Shader create(const BYTE* shader_bytecode, size_t bytecode_size);
		static Pixel_Shader create(PShader_ID pixel_shader_id);

		void free_shader();

		HRESULT bind_texture_2d(const char* binding_name, const Shader_Texture2D& texture);
		HRESULT bind_constant_buffer(const char* binding_name, ID3D11Buffer* const constant_buffer);

		static ID3D11PixelShader* s_registered_pixel_shaders[(uint8_t)PShader_ID::NUM_SHADERS];
		static Shader_Bytecode s_pixel_shader_bytecode_buffers[(uint8_t)PShader_ID::NUM_SHADERS];
	};


	struct Vertex_Shader
	{
		ID3D11VertexShader* m_shader;
		ID3D11InputLayout* m_vertex_input_layout;
		ID3D11ShaderReflection* m_shader_reflection;

		static Vertex_Shader create(const BYTE* shader_bytecode, size_t bytecode_size, D3D11_INPUT_ELEMENT_DESC* vertex_layout_desc, int num_input_elements);
		static Vertex_Shader create(VShader_ID vertex_shader_id, D3D11_INPUT_ELEMENT_DESC* vertex_layout_desc, int num_input_elements);

		void free_shader();

		HRESULT bind_constant_buffer(const char* binding_name, ID3D11Buffer* const constant_buffer);

		static ID3D11VertexShader* s_registered_vertex_shaders[(uint8_t)VShader_ID::NUM_SHADERS];
		static Shader_Bytecode s_vertex_shader_bytecode_buffers[(uint8_t)VShader_ID::NUM_SHADERS];
	};
}

#endif