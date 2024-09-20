#include "PipelineStateManager.h"
#include "Platform.h"

#include "renderer/backend/rhi/RenderHardwareInterface.h"
#include "renderer/backend/PipelineStateObject.h"

namespace zorya
{
	Pipeline_State_Manager pipeline_state_manager;

	Pipeline_State_Manager::Pipeline_State_Manager()
	{
		m_shading_models.resize(Shading_Model::NUM_SHADING_MODELS);
		m_pipeline_states.resize(Pipeline_State::NUM_PIPELINE_STATES);
	}

	HRESULT Pipeline_State_Manager::init()
	{
		{
			PSO_Desc desc = create_default_gbuff_desc();
			
			zassert(rhi.create_pso(&m_shading_models[Shading_Model::DEFAULT_LIT], desc).value == S_OK);
		}
		
		{
			PSO_Desc desc = create_default_gbuff_desc();
			desc.stencil_ref_value = 2;
			
			zassert(rhi.create_pso(&m_shading_models[Shading_Model::SUBSURFACE_GOLUBEV], desc).value == S_OK);
		}

		{
			PSO_Desc shadow_map_creation_pso_desc;
			shadow_map_creation_pso_desc.pixel_shader_bytecode = Shader_Bytecode{ nullptr, 0 };
			shadow_map_creation_pso_desc.vertex_shader_bytecode = Shader_Manager::s_vertex_shader_bytecode_buffers[(uint8_t)VShader_ID::DEPTH];
			shadow_map_creation_pso_desc.depth_stencil_desc.StencilEnable = false;
			shadow_map_creation_pso_desc.depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			shadow_map_creation_pso_desc.depth_stencil_desc.DepthEnable = true;
			shadow_map_creation_pso_desc.depth_stencil_desc.DepthFunc = D3D11_COMPARISON_GREATER;
			shadow_map_creation_pso_desc.rasterizer_desc = default_rs_desc;
			shadow_map_creation_pso_desc.input_elements_desc = s_vertex_layout_desc;
			shadow_map_creation_pso_desc.num_elements = sizeof(s_vertex_layout_desc)/sizeof(s_vertex_layout_desc[0]);
			shadow_map_creation_pso_desc.blend_desc = create_default_blend_desc();

			zassert(rhi.create_pso(&m_pipeline_states[Pipeline_State::SHADOW_MAPPING], shadow_map_creation_pso_desc).value == S_OK);
		}

		{
			PSO_Desc skybox_pso_desc;
			skybox_pso_desc.vertex_shader_bytecode = Shader_Manager::s_vertex_shader_bytecode_buffers[(uint8_t)VShader_ID::SKYBOX];
			skybox_pso_desc.pixel_shader_bytecode = Shader_Manager::s_pixel_shader_bytecode_buffers[(uint8_t)PShader_ID::SKYBOX];
			skybox_pso_desc.depth_stencil_desc = default_ds_desc;
			skybox_pso_desc.depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
			skybox_pso_desc.rasterizer_desc = default_rs_desc;
			skybox_pso_desc.input_elements_desc = s_vertex_layout_desc;
			skybox_pso_desc.num_elements = sizeof(s_vertex_layout_desc) / sizeof(s_vertex_layout_desc[0]);
			skybox_pso_desc.blend_desc = create_default_blend_desc();

			zassert(rhi.create_pso(&m_pipeline_states[Pipeline_State::SKYBOX], skybox_pso_desc).value == S_OK);
		}

		{
			PSO_Desc composit_pso_desc;
			composit_pso_desc.vertex_shader_bytecode = Shader_Manager::s_vertex_shader_bytecode_buffers[(uint8_t)VShader_ID::FULL_QUAD];
			composit_pso_desc.pixel_shader_bytecode = Shader_Manager::s_pixel_shader_bytecode_buffers[(uint8_t)PShader_ID::COMPOSIT];
			composit_pso_desc.depth_stencil_desc = default_ds_desc;
			composit_pso_desc.rasterizer_desc = default_rs_desc;
			composit_pso_desc.input_elements_desc = nullptr;
			composit_pso_desc.num_elements = 0;
			composit_pso_desc.blend_desc = create_default_blend_desc();

			zassert(rhi.create_pso(&m_pipeline_states[Pipeline_State::COMPOSIT], composit_pso_desc).value == S_OK);
		}

		{
			PSO_Desc shadow_mask_build_pso_desc;
			shadow_mask_build_pso_desc.pixel_shader_bytecode = Shader_Manager::s_pixel_shader_bytecode_buffers[(uint8_t)PShader_ID::SHADOW_MASK];
			shadow_mask_build_pso_desc.vertex_shader_bytecode = Shader_Manager::s_vertex_shader_bytecode_buffers[(uint8_t)VShader_ID::FULL_QUAD];
			shadow_mask_build_pso_desc.depth_stencil_desc = default_ds_desc;
			shadow_mask_build_pso_desc.blend_desc = create_default_blend_desc();
			shadow_mask_build_pso_desc.rasterizer_desc = default_rs_desc;
			shadow_mask_build_pso_desc.input_elements_desc = nullptr;
			shadow_mask_build_pso_desc.num_elements = 0;
			shadow_mask_build_pso_desc.stencil_ref_value = 1;

			shadow_mask_build_pso_desc.depth_stencil_desc.DepthEnable = false;
			shadow_mask_build_pso_desc.depth_stencil_desc.StencilEnable = true;
			shadow_mask_build_pso_desc.depth_stencil_desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
			shadow_mask_build_pso_desc.depth_stencil_desc.StencilWriteMask = 0x00;
			shadow_mask_build_pso_desc.depth_stencil_desc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
			shadow_mask_build_pso_desc.depth_stencil_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			shadow_mask_build_pso_desc.depth_stencil_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;

			zassert(rhi.create_pso(&m_pipeline_states[Pipeline_State::SHADOW_MASK], shadow_mask_build_pso_desc).value == S_OK);
		}

		{
			PSO_Desc lighting_pso_desc;
			lighting_pso_desc.pixel_shader_bytecode = Shader_Manager::s_pixel_shader_bytecode_buffers[(uint8_t)PShader_ID::LIGHTING];
			lighting_pso_desc.vertex_shader_bytecode = Shader_Manager::s_vertex_shader_bytecode_buffers[(uint8_t)VShader_ID::FULL_QUAD];
			lighting_pso_desc.depth_stencil_desc = default_ds_desc;
			lighting_pso_desc.rasterizer_desc = default_rs_desc;
			lighting_pso_desc.input_elements_desc = nullptr;
			lighting_pso_desc.num_elements = 0;

			lighting_pso_desc.depth_stencil_desc.DepthEnable = false;
			lighting_pso_desc.depth_stencil_desc.StencilEnable = true;
			lighting_pso_desc.depth_stencil_desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
			lighting_pso_desc.depth_stencil_desc.StencilWriteMask = 0x00;
			lighting_pso_desc.depth_stencil_desc.FrontFace.StencilFunc = D3D11_COMPARISON_LESS_EQUAL;
			lighting_pso_desc.depth_stencil_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			lighting_pso_desc.depth_stencil_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;

			lighting_pso_desc.stencil_ref_value = 1;

			lighting_pso_desc.blend_desc.AlphaToCoverageEnable = FALSE;
			lighting_pso_desc.blend_desc.IndependentBlendEnable = FALSE;
			lighting_pso_desc.blend_desc.RenderTarget[0].BlendEnable = true;
			lighting_pso_desc.blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			lighting_pso_desc.blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			lighting_pso_desc.blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
			lighting_pso_desc.blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
			lighting_pso_desc.blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			lighting_pso_desc.blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
			lighting_pso_desc.blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
			lighting_pso_desc.blend_desc.RenderTarget[1].BlendEnable = true;
			lighting_pso_desc.blend_desc.RenderTarget[2].BlendEnable = true;

			zassert(rhi.create_pso(&m_pipeline_states[Pipeline_State::LIGHTING], lighting_pso_desc).value == S_OK);
		}

		{
			PSO_Desc sss_golubev_pso_desc;
			sss_golubev_pso_desc.pixel_shader_bytecode = Shader_Manager::s_pixel_shader_bytecode_buffers[(u8)PShader_ID::SSSSS];
			sss_golubev_pso_desc.vertex_shader_bytecode = Shader_Manager::s_vertex_shader_bytecode_buffers[(uint8_t)VShader_ID::FULL_QUAD];
			sss_golubev_pso_desc.depth_stencil_desc = default_ds_desc;
			sss_golubev_pso_desc.rasterizer_desc = default_rs_desc;
			sss_golubev_pso_desc.input_elements_desc = nullptr;
			sss_golubev_pso_desc.num_elements = 0;

			sss_golubev_pso_desc.depth_stencil_desc.DepthEnable = false;
			sss_golubev_pso_desc.depth_stencil_desc.StencilEnable = true;
			sss_golubev_pso_desc.depth_stencil_desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
			sss_golubev_pso_desc.depth_stencil_desc.StencilWriteMask = 0x00;
			sss_golubev_pso_desc.depth_stencil_desc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
			sss_golubev_pso_desc.depth_stencil_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			sss_golubev_pso_desc.depth_stencil_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;


			sss_golubev_pso_desc.stencil_ref_value = 2;

			sss_golubev_pso_desc.blend_desc = D3D11_BLEND_DESC{};
			sss_golubev_pso_desc.blend_desc.AlphaToCoverageEnable = FALSE;
			sss_golubev_pso_desc.blend_desc.IndependentBlendEnable = FALSE;
			sss_golubev_pso_desc.blend_desc.RenderTarget[0].BlendEnable = false;
			sss_golubev_pso_desc.blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			zassert(rhi.create_pso(&m_pipeline_states[Pipeline_State::SSS_GOLUBEV], sss_golubev_pso_desc).value == S_OK);
		}

		return S_OK;
	}

	void Pipeline_State_Manager::shutdown()
	{
		m_shading_models.clear();
		m_pipeline_states.clear();
	}


	PSO_Handle Pipeline_State_Manager::get(Shading_Model shading_model_id) const
	{
		return m_shading_models[shading_model_id];
	}

	PSO_Handle Pipeline_State_Manager::get(Pipeline_State pipeline_state_id) const
	{
		return m_pipeline_states[pipeline_state_id];
	}
}