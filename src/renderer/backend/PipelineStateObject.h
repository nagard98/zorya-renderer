#ifndef PIPELINE_STATE_OBJECT_H_
#define PIPELINE_STATE_OBJECT_H_

#include "d3d11.h"
#include "Platform.h"
#include "renderer/frontend/Shader.h"
#include "renderer/backend/rhi/RenderDeviceHandles.h"

#include <cstdint>

#define MAX_NUM_ROOT_PARAMS 6

namespace zorya
{
	enum class Shader_Visibility : uint8_t
	{
		PIXEL_SHADER,
		VERTEX_SHADER
	};

	enum class Shader_View_Type : uint8_t
	{
		CBV,
		SRV,
		UAV
	};

	struct Root_Param
	{
		Shader_View_Type view_type;
		uint8_t base_register;
		uint8_t num_resources;
		Shader_Visibility visibility;
	};

	struct Root_Signature
	{
		Root_Param root_params[MAX_NUM_ROOT_PARAMS];
	};

	struct PSO_Desc
	{
		Shader_Bytecode pixel_shader_bytecode;
		Shader_Bytecode vertex_shader_bytecode;
		D3D11_RASTERIZER_DESC rasterizer_desc;
		D3D11_DEPTH_STENCIL_DESC depth_stencil_desc;
		D3D11_INPUT_ELEMENT_DESC* input_elements_desc;
		D3D11_BLEND_DESC blend_desc;
		uint8_t num_elements;
		uint8_t stencil_ref_value;
		Root_Signature *root_signature;
	};

	uint64_t hash(const PSO_Desc& pso_desc);

	struct Pipeline_State_Object
	{
		ID3D11PixelShader* pixel_shader;
		ID3D11VertexShader* vertex_shader;
		ID3D11DepthStencilState* ds_state;
		ID3D11RasterizerState* rs_state;
		ID3D11BlendState* blend_state;
		ID3D11InputLayout* input_layout;
		uint8_t stencil_ref_value;
	};

}

//namespace std
//{
//	//TODO
//	template<>
//	struct hash<uint64_t>
//	{
//		uint64_t operator()(const uint64_t& val)
//		{
//			return val;
//		}
//	};
//}

#endif