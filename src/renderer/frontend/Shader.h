#ifndef ZORYA_SHADER_H_
#define ZORYA_SHADER_H_

#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <utils/Arena.h>

#include <reflection/ReflectionGenerationUtils.h>
#include "Asset.h"
#include "Texture2D.h"

#include "renderer/backend/rhi/RenderDeviceHandles.h"

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
		COMPOSIT,
		SHADOW_MASK,
		EQUIRECTANGULAR,
		CONVOLVE_CUBEMAP,
		BUILD_PRE_FILTERED_CUBEMAP,
		NUM_SHADERS
	};


	static D3D11_INPUT_ELEMENT_DESC s_vertex_layout_desc[4] = {
		{ "position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "texcoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "tangent", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};


	struct Shader_Bytecode
	{
		const BYTE* bytecode;
		size_t size_in_bytes;
	};

	using Shader_Texture2D = ID3D11ShaderResourceView;
	

	enum Resource_Type : uint8_t
	{
		ZRY_TEX = D3D_SIT_TEXTURE,
		ZRY_CBUFF = D3D_SIT_CBUFFER,
		ZRY_UNSUPPORTED
	};

	struct CB_Var_Desc
	{
		VAR_REFL_TYPE variable_type;
		uint32_t columns;
		uint32_t elements;
	};

	struct CB_Variable
	{
		char* name;
		CB_Var_Desc description;
		uint32_t offset_in_bytes;
		uint32_t size_in_bytes;
	};

	struct Constant_Buffer_Data
	{
		CB_Variable* variables;
		uint32_t num_variables;
		void* cb_start;
	};

	struct Shader_Arguments
	{
		Constant_Buffer_Handle* vs_cb_handles;
		uint32_t num_vs_cb_handles;

		Render_SRV_Handle* vs_srv_handles;
		uint32_t num_vs_srv_handles;

		Constant_Buffer_Handle* ps_cb_handles;
		uint32_t num_ps_cb_handles;

		Render_SRV_Handle* ps_srv_handles;
		uint32_t num_ps_srv_handles;
	};


	struct Shader_Resource
	{
		Shader_Resource(const char* name, uint8_t bind_point, uint8_t bind_count, Resource_Type type, Render_Resource_Handle hnd_resource, void* data);

		char* m_name;
		uint8_t m_bind_point;
		uint8_t m_bind_count;
		Resource_Type m_type;
		union
		{
			Render_SRV_Handle m_hnd_gpu_srv;
			Constant_Buffer_Handle m_hnd_gpu_cb;
		};
		//uint32_t m_hnd_resource;
		//void* m_fake_hnd_resource;

		union
		{
			Texture2D* m_texture;
			Constant_Buffer_Data* m_parameters;
		};
		
	};

	Shader_Arguments create_shader_arguments(Arena& arena, const std::vector<Shader_Resource>& shader_resources);

	Shader_Arguments create_shader_arguments(Arena& arena, Constant_Buffer_Handle* arr_vs_cb_hnds, uint32_t num_vs_cb_hnds, Render_SRV_Handle* arr_vs_srv_hnds, uint32_t num_vs_srv_hnds, Constant_Buffer_Handle* arr_ps_cb_hnds, uint32_t num_ps_cb_hnds, Render_SRV_Handle* arr_ps_srv_hnds, uint32_t num_ps_srv_hnds);

	template <uint32_t num_vs_cb_hnds, uint32_t num_vs_srv_hnds, uint32_t num_ps_cb_hnds, uint32_t num_ps_srv_hnds>
	Shader_Arguments create_shader_arguments(Arena& arena, Constant_Buffer_Handle (&arr_vs_cb_hnds)[num_vs_cb_hnds], Render_SRV_Handle (&arr_vs_srv_hnds)[num_vs_srv_hnds], Constant_Buffer_Handle(&arr_ps_cb_hnds)[num_ps_cb_hnds], Render_SRV_Handle(&arr_ps_srv_hnds)[num_ps_srv_hnds])
	{
		return create_shader_arguments(arena, arr_vs_cb_hnds, num_vs_cb_hnds, arr_vs_srv_hnds, num_vs_srv_hnds, arr_ps_cb_hnds, num_ps_cb_hnds, arr_ps_srv_hnds, num_ps_srv_hnds);
	}

	struct Pixel_Shader_Handle
	{
		uint32_t index;
	};

	struct Vertex_Shader_Handle
	{
		uint32_t index;
	};

	struct Shader_Manager
	{
		Shader_Manager();

		Pixel_Shader_Handle ps_hnd_from_bytecode(const Shader_Bytecode& bytecode);
		Vertex_Shader_Handle vs_hnd_from_bytecode(const Shader_Bytecode& bytecode);

		std::unordered_map<uint64_t, Pixel_Shader_Handle> ps_handles;
		std::unordered_map<uint64_t, Vertex_Shader_Handle> vs_handles;

		static Shader_Bytecode s_pixel_shader_bytecode_buffers[(uint8_t)PShader_ID::NUM_SHADERS];
		static Shader_Bytecode s_vertex_shader_bytecode_buffers[(uint8_t)VShader_ID::NUM_SHADERS];
	};

	extern Shader_Manager shader_manager;

	struct Pixel_Shader
	{
		ID3D11PixelShader* m_shader;
		ID3D11ShaderReflection* m_shader_reflection;

		static Pixel_Shader create(const BYTE* shader_bytecode, size_t bytecode_size);
		static Pixel_Shader create(PShader_ID pixel_shader_id);

		std::vector<Shader_Resource> build_uniforms_struct();
		void free_shader();

		HRESULT bind_texture_2d(const char* binding_name, Shader_Texture2D** const texture);
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


	bool set_shader_resource_asset(std::vector<Shader_Resource>& resources, const char* resource_name, Texture2D* tex_asset, const Texture_Import_Config* tex_imp_conf);

	bool set_constant_buff_var(Constant_Buffer_Data* cb_data, const char* var_name, void* value, size_t val_size_in_bytes);
	bool get_constant_buff_var(Constant_Buffer_Data* cb_data, const char* var_name, void* value);
}

#endif