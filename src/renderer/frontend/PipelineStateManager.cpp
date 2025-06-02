#include "PipelineStateManager.h"
#include "Platform.h"

#include "renderer/backend/rhi/RenderHardwareInterface.h"
#include "renderer/backend/PipelineStateObject.h"

namespace zorya
{
	static D3D11_BLEND_DESC create_default_blend_desc();
	static PSO_Desc create_default_gbuff_desc();

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
			PSO_Desc desc = create_default_gbuff_desc();
			desc.stencil_ref_value = 3;
			zassert(rhi.create_pso(&m_shading_models[Shading_Model::SUBSURFACE_JIMENEZ_GAUSS], desc).value == S_OK);
		}

		{
			PSO_Desc desc = create_default_gbuff_desc();
			desc.stencil_ref_value = 4;
			zassert(rhi.create_pso(&m_shading_models[Shading_Model::SUBSURFACE_JIMENEZ_SEPARABLE], desc).value == S_OK);
		}

		{
			PSO_Desc shadow_map_creation_pso_desc;
			shadow_map_creation_pso_desc.pixel_shader_bytecode = Shader_Bytecode{ nullptr, 0 };
			shadow_map_creation_pso_desc.vertex_shader_bytecode = Shader_Manager::s_vertex_shader_bytecode_buffers[(uint8_t)VShader_ID::DEPTH];
			shadow_map_creation_pso_desc.depth_stencil_desc.stencil_enable = false;
			shadow_map_creation_pso_desc.depth_stencil_desc.depth_write_mask = D3D11_DEPTH_WRITE_MASK_ALL;
			shadow_map_creation_pso_desc.depth_stencil_desc.depth_enable = true;
			shadow_map_creation_pso_desc.depth_stencil_desc.depth_test_func = Comparison_Func::GREATER;
			shadow_map_creation_pso_desc.rasterizer_desc = Rasterizer_State_Desc::create();
			shadow_map_creation_pso_desc.input_elements_desc = s_vertex_layout_desc;
			shadow_map_creation_pso_desc.num_elements = sizeof(s_vertex_layout_desc)/sizeof(s_vertex_layout_desc[0]);
			shadow_map_creation_pso_desc.blend_desc = create_default_blend_desc();

			zassert(rhi.create_pso(&m_pipeline_states[Pipeline_State::SHADOW_MAPPING], shadow_map_creation_pso_desc).value == S_OK);
		}

		{
			PSO_Desc skybox_pso_desc;
			skybox_pso_desc.vertex_shader_bytecode = Shader_Manager::s_vertex_shader_bytecode_buffers[(uint8_t)VShader_ID::SKYBOX];
			skybox_pso_desc.pixel_shader_bytecode = Shader_Manager::s_pixel_shader_bytecode_buffers[(uint8_t)PShader_ID::SKYBOX];
			skybox_pso_desc.depth_stencil_desc = Depth_Stencil_State_Desc::create();
			skybox_pso_desc.depth_stencil_desc.depth_test_func = Comparison_Func::LESS_EQUAL;
			skybox_pso_desc.rasterizer_desc = Rasterizer_State_Desc::create();
			skybox_pso_desc.input_elements_desc = s_vertex_layout_desc;
			skybox_pso_desc.num_elements = sizeof(s_vertex_layout_desc) / sizeof(s_vertex_layout_desc[0]);
			skybox_pso_desc.blend_desc = create_default_blend_desc();

			zassert(rhi.create_pso(&m_pipeline_states[Pipeline_State::SKYBOX], skybox_pso_desc).value == S_OK);
		}

		{
			PSO_Desc equirectagular_to_cubemap_pso_desc;
			equirectagular_to_cubemap_pso_desc.vertex_shader_bytecode = Shader_Manager::s_vertex_shader_bytecode_buffers[(uint8_t)VShader_ID::SKYBOX];
			equirectagular_to_cubemap_pso_desc.pixel_shader_bytecode = Shader_Manager::s_pixel_shader_bytecode_buffers[(uint8_t)PShader_ID::EQUIRECTANGULAR];
			equirectagular_to_cubemap_pso_desc.depth_stencil_desc = Depth_Stencil_State_Desc::create();
			equirectagular_to_cubemap_pso_desc.depth_stencil_desc.depth_test_func = Comparison_Func::LESS_EQUAL;
			equirectagular_to_cubemap_pso_desc.rasterizer_desc = Rasterizer_State_Desc::create();
			equirectagular_to_cubemap_pso_desc.input_elements_desc = s_vertex_layout_desc;
			equirectagular_to_cubemap_pso_desc.num_elements = sizeof(s_vertex_layout_desc) / sizeof(s_vertex_layout_desc[0]);
			equirectagular_to_cubemap_pso_desc.blend_desc = create_default_blend_desc();

			zassert(rhi.create_pso(&m_pipeline_states[Pipeline_State::EQUIRECTANGULAR_CONVERSION], equirectagular_to_cubemap_pso_desc).value == S_OK);
		}
		
		{
			PSO_Desc convolve_cubemap_pso_desc;
			convolve_cubemap_pso_desc.vertex_shader_bytecode = Shader_Manager::s_vertex_shader_bytecode_buffers[(uint8_t)VShader_ID::SKYBOX];
			convolve_cubemap_pso_desc.pixel_shader_bytecode = Shader_Manager::s_pixel_shader_bytecode_buffers[(uint8_t)PShader_ID::CONVOLVE_CUBEMAP];
			convolve_cubemap_pso_desc.depth_stencil_desc = Depth_Stencil_State_Desc::create();
			convolve_cubemap_pso_desc.depth_stencil_desc.depth_test_func = Comparison_Func::LESS_EQUAL;
			convolve_cubemap_pso_desc.rasterizer_desc = Rasterizer_State_Desc::create();
			convolve_cubemap_pso_desc.input_elements_desc = s_vertex_layout_desc;
			convolve_cubemap_pso_desc.num_elements = sizeof(s_vertex_layout_desc) / sizeof(s_vertex_layout_desc[0]);
			convolve_cubemap_pso_desc.blend_desc = create_default_blend_desc();

			zassert(rhi.create_pso(&m_pipeline_states[Pipeline_State::CONVOLVE_CUBEMAP], convolve_cubemap_pso_desc).value == S_OK);
		}

		{
			PSO_Desc build_pre_filtered_map_pso_desc;
			build_pre_filtered_map_pso_desc.vertex_shader_bytecode = Shader_Manager::s_vertex_shader_bytecode_buffers[(uint8_t)VShader_ID::SKYBOX];
			build_pre_filtered_map_pso_desc.pixel_shader_bytecode = Shader_Manager::s_pixel_shader_bytecode_buffers[(uint8_t)PShader_ID::BUILD_PRE_FILTERED_CUBEMAP];
			build_pre_filtered_map_pso_desc.depth_stencil_desc = Depth_Stencil_State_Desc::create();
			build_pre_filtered_map_pso_desc.depth_stencil_desc.depth_test_func = Comparison_Func::LESS_EQUAL;
			build_pre_filtered_map_pso_desc.rasterizer_desc = Rasterizer_State_Desc::create();
			build_pre_filtered_map_pso_desc.input_elements_desc = s_vertex_layout_desc;
			build_pre_filtered_map_pso_desc.num_elements = sizeof(s_vertex_layout_desc) / sizeof(s_vertex_layout_desc[0]);
			build_pre_filtered_map_pso_desc.blend_desc = create_default_blend_desc();

			zassert(rhi.create_pso(&m_pipeline_states[Pipeline_State::BUILD_PRE_FILTERED_MAP], build_pre_filtered_map_pso_desc).value == S_OK);
		}
		{
			PSO_Desc build_brdf_lut_map_pso_desc;
			build_brdf_lut_map_pso_desc.vertex_shader_bytecode = Shader_Manager::s_vertex_shader_bytecode_buffers[(uint8_t)VShader_ID::FULL_QUAD];
			build_brdf_lut_map_pso_desc.pixel_shader_bytecode = Shader_Manager::s_pixel_shader_bytecode_buffers[(uint8_t)PShader_ID::BUILD_PRE_FILTERED_CUBEMAP];
			build_brdf_lut_map_pso_desc.depth_stencil_desc = Depth_Stencil_State_Desc::create();
			build_brdf_lut_map_pso_desc.rasterizer_desc = Rasterizer_State_Desc::create();
			build_brdf_lut_map_pso_desc.input_elements_desc = nullptr;
			build_brdf_lut_map_pso_desc.num_elements = 0;
			build_brdf_lut_map_pso_desc.blend_desc = create_default_blend_desc();

			zassert(rhi.create_pso(&m_pipeline_states[Pipeline_State::BUILD_BRDF_LUT_MAP], build_brdf_lut_map_pso_desc).value == S_OK);
		}

		{
			PSO_Desc composit_pso_desc;
			composit_pso_desc.vertex_shader_bytecode = Shader_Manager::s_vertex_shader_bytecode_buffers[(uint8_t)VShader_ID::FULL_QUAD];
			composit_pso_desc.pixel_shader_bytecode = Shader_Manager::s_pixel_shader_bytecode_buffers[(uint8_t)PShader_ID::COMPOSIT];
			composit_pso_desc.depth_stencil_desc = Depth_Stencil_State_Desc::create();
			composit_pso_desc.rasterizer_desc = Rasterizer_State_Desc::create();
			composit_pso_desc.input_elements_desc = nullptr;
			composit_pso_desc.num_elements = 0;
			composit_pso_desc.blend_desc = create_default_blend_desc();

			zassert(rhi.create_pso(&m_pipeline_states[Pipeline_State::COMPOSIT], composit_pso_desc).value == S_OK);
		}

		{
			PSO_Desc shadow_mask_build_pso_desc;
			shadow_mask_build_pso_desc.pixel_shader_bytecode = Shader_Manager::s_pixel_shader_bytecode_buffers[(uint8_t)PShader_ID::SHADOW_MASK];
			shadow_mask_build_pso_desc.vertex_shader_bytecode = Shader_Manager::s_vertex_shader_bytecode_buffers[(uint8_t)VShader_ID::FULL_QUAD];
			shadow_mask_build_pso_desc.depth_stencil_desc = Depth_Stencil_State_Desc::create();
			shadow_mask_build_pso_desc.blend_desc = create_default_blend_desc();
			shadow_mask_build_pso_desc.rasterizer_desc = Rasterizer_State_Desc::create();
			shadow_mask_build_pso_desc.input_elements_desc = nullptr;
			shadow_mask_build_pso_desc.num_elements = 0;
			shadow_mask_build_pso_desc.stencil_ref_value = 1;

			shadow_mask_build_pso_desc.depth_stencil_desc.depth_enable = false;
			shadow_mask_build_pso_desc.depth_stencil_desc.stencil_enable = true;
			shadow_mask_build_pso_desc.depth_stencil_desc.stencil_read_mask = D3D11_DEFAULT_STENCIL_READ_MASK;
			shadow_mask_build_pso_desc.depth_stencil_desc.stencil_write_mask = 0x00;
			shadow_mask_build_pso_desc.depth_stencil_desc.front_face.stencil_test_func = Comparison_Func::EQUAL;
			shadow_mask_build_pso_desc.depth_stencil_desc.front_face.stencil_fail_op = Stencil_Op::KEEP;
			shadow_mask_build_pso_desc.depth_stencil_desc.front_face.stencil_depth_pass_op = Stencil_Op::KEEP;

			zassert(rhi.create_pso(&m_pipeline_states[Pipeline_State::SHADOW_MASK], shadow_mask_build_pso_desc).value == S_OK);
		}

		{
			PSO_Desc lighting_pso_desc;
			lighting_pso_desc.pixel_shader_bytecode = Shader_Manager::s_pixel_shader_bytecode_buffers[(uint8_t)PShader_ID::LIGHTING];
			lighting_pso_desc.vertex_shader_bytecode = Shader_Manager::s_vertex_shader_bytecode_buffers[(uint8_t)VShader_ID::FULL_QUAD];
			lighting_pso_desc.depth_stencil_desc = Depth_Stencil_State_Desc::create();
			lighting_pso_desc.rasterizer_desc = Rasterizer_State_Desc::create();
			lighting_pso_desc.input_elements_desc = nullptr;
			lighting_pso_desc.num_elements = 0;

			lighting_pso_desc.depth_stencil_desc.depth_enable = false;
			lighting_pso_desc.depth_stencil_desc.stencil_enable = true;
			lighting_pso_desc.depth_stencil_desc.stencil_read_mask = D3D11_DEFAULT_STENCIL_READ_MASK;
			lighting_pso_desc.depth_stencil_desc.stencil_write_mask = 0x00;
			lighting_pso_desc.depth_stencil_desc.front_face.stencil_test_func = Comparison_Func::LESS_EQUAL;
			lighting_pso_desc.depth_stencil_desc.front_face.stencil_fail_op = Stencil_Op::KEEP;
			lighting_pso_desc.depth_stencil_desc.front_face.stencil_depth_pass_op = Stencil_Op::KEEP;

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
			sss_golubev_pso_desc.depth_stencil_desc = Depth_Stencil_State_Desc::create();
			sss_golubev_pso_desc.rasterizer_desc = Rasterizer_State_Desc::create();
			sss_golubev_pso_desc.input_elements_desc = nullptr;
			sss_golubev_pso_desc.num_elements = 0;

			sss_golubev_pso_desc.depth_stencil_desc.depth_enable = false;
			sss_golubev_pso_desc.depth_stencil_desc.stencil_enable = true;
			sss_golubev_pso_desc.depth_stencil_desc.stencil_read_mask = D3D11_DEFAULT_STENCIL_READ_MASK;
			sss_golubev_pso_desc.depth_stencil_desc.stencil_write_mask = 0x00;
			sss_golubev_pso_desc.depth_stencil_desc.front_face.stencil_test_func = Comparison_Func::EQUAL;
			sss_golubev_pso_desc.depth_stencil_desc.front_face.stencil_fail_op = Stencil_Op::KEEP;
			sss_golubev_pso_desc.depth_stencil_desc.front_face.stencil_depth_pass_op = Stencil_Op::KEEP;

			sss_golubev_pso_desc.stencil_ref_value = 2;

			sss_golubev_pso_desc.blend_desc = D3D11_BLEND_DESC{};
			sss_golubev_pso_desc.blend_desc.AlphaToCoverageEnable = FALSE;
			sss_golubev_pso_desc.blend_desc.IndependentBlendEnable = FALSE;
			sss_golubev_pso_desc.blend_desc.RenderTarget[0].BlendEnable = false;
			sss_golubev_pso_desc.blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			zassert(rhi.create_pso(&m_pipeline_states[Pipeline_State::SSS_GOLUBEV], sss_golubev_pso_desc).value == S_OK);
		}

		{
			PSO_Desc sss_jimenez_sep_pso_desc;
			sss_jimenez_sep_pso_desc.pixel_shader_bytecode = Shader_Manager::s_pixel_shader_bytecode_buffers[(u8)PShader_ID::SSSSS];
			sss_jimenez_sep_pso_desc.vertex_shader_bytecode = Shader_Manager::s_vertex_shader_bytecode_buffers[(uint8_t)VShader_ID::FULL_QUAD];
			sss_jimenez_sep_pso_desc.depth_stencil_desc = Depth_Stencil_State_Desc::create();
			sss_jimenez_sep_pso_desc.rasterizer_desc = Rasterizer_State_Desc::create();
			sss_jimenez_sep_pso_desc.input_elements_desc = nullptr;
			sss_jimenez_sep_pso_desc.num_elements = 0;

			sss_jimenez_sep_pso_desc.depth_stencil_desc.depth_enable = false;
			sss_jimenez_sep_pso_desc.depth_stencil_desc.stencil_enable = true;
			sss_jimenez_sep_pso_desc.depth_stencil_desc.stencil_read_mask = D3D11_DEFAULT_STENCIL_READ_MASK;
			sss_jimenez_sep_pso_desc.depth_stencil_desc.stencil_write_mask = 0x00;
			sss_jimenez_sep_pso_desc.depth_stencil_desc.front_face.stencil_test_func = Comparison_Func::EQUAL;
			sss_jimenez_sep_pso_desc.depth_stencil_desc.front_face.stencil_fail_op = Stencil_Op::KEEP;
			sss_jimenez_sep_pso_desc.depth_stencil_desc.front_face.stencil_depth_pass_op = Stencil_Op::KEEP;

			sss_jimenez_sep_pso_desc.stencil_ref_value = 4;

			sss_jimenez_sep_pso_desc.blend_desc = D3D11_BLEND_DESC{};
			sss_jimenez_sep_pso_desc.blend_desc.AlphaToCoverageEnable = FALSE;
			sss_jimenez_sep_pso_desc.blend_desc.IndependentBlendEnable = FALSE;
			sss_jimenez_sep_pso_desc.blend_desc.RenderTarget[0].BlendEnable = false;
			sss_jimenez_sep_pso_desc.blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			zassert(rhi.create_pso(&m_pipeline_states[Pipeline_State::SSS_JIMENEZ_SEPARABLE], sss_jimenez_sep_pso_desc).value == S_OK);
		}

		{
			PSO_Desc sss_jimenez_gauss_hor_pso_desc;
			sss_jimenez_gauss_hor_pso_desc.pixel_shader_bytecode = Shader_Manager::s_pixel_shader_bytecode_buffers[(u8)PShader_ID::SSSSS];
			sss_jimenez_gauss_hor_pso_desc.vertex_shader_bytecode = Shader_Manager::s_vertex_shader_bytecode_buffers[(uint8_t)VShader_ID::FULL_QUAD];
			sss_jimenez_gauss_hor_pso_desc.depth_stencil_desc = Depth_Stencil_State_Desc::create();
			sss_jimenez_gauss_hor_pso_desc.rasterizer_desc = Rasterizer_State_Desc::create();
			sss_jimenez_gauss_hor_pso_desc.input_elements_desc = nullptr;
			sss_jimenez_gauss_hor_pso_desc.num_elements = 0;

			sss_jimenez_gauss_hor_pso_desc.depth_stencil_desc.depth_enable = false;
			sss_jimenez_gauss_hor_pso_desc.depth_stencil_desc.stencil_enable = true;
			sss_jimenez_gauss_hor_pso_desc.depth_stencil_desc.stencil_read_mask = D3D11_DEFAULT_STENCIL_READ_MASK;
			sss_jimenez_gauss_hor_pso_desc.depth_stencil_desc.stencil_write_mask = 0x00;
			sss_jimenez_gauss_hor_pso_desc.depth_stencil_desc.front_face.stencil_test_func = Comparison_Func::EQUAL;
			sss_jimenez_gauss_hor_pso_desc.depth_stencil_desc.front_face.stencil_fail_op = Stencil_Op::KEEP;
			sss_jimenez_gauss_hor_pso_desc.depth_stencil_desc.front_face.stencil_depth_pass_op = Stencil_Op::KEEP;

			sss_jimenez_gauss_hor_pso_desc.stencil_ref_value = 3;

			sss_jimenez_gauss_hor_pso_desc.blend_desc = D3D11_BLEND_DESC{};
			sss_jimenez_gauss_hor_pso_desc.blend_desc.AlphaToCoverageEnable = FALSE;
			sss_jimenez_gauss_hor_pso_desc.blend_desc.IndependentBlendEnable = FALSE;
			sss_jimenez_gauss_hor_pso_desc.blend_desc.RenderTarget[0].BlendEnable = true;
			sss_jimenez_gauss_hor_pso_desc.blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			sss_jimenez_gauss_hor_pso_desc.blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_BLEND_FACTOR;
			sss_jimenez_gauss_hor_pso_desc.blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_BLEND_FACTOR;
			sss_jimenez_gauss_hor_pso_desc.blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			sss_jimenez_gauss_hor_pso_desc.blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
			sss_jimenez_gauss_hor_pso_desc.blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			sss_jimenez_gauss_hor_pso_desc.blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			zassert(rhi.create_pso(&m_pipeline_states[Pipeline_State::SSS_JIMENEZ_GAUSS_VERT], sss_jimenez_gauss_hor_pso_desc).value == S_OK);
		}
		
		{
			PSO_Desc sss_jimenez_gauss_ver_pso_desc;
			sss_jimenez_gauss_ver_pso_desc.pixel_shader_bytecode = Shader_Manager::s_pixel_shader_bytecode_buffers[(u8)PShader_ID::SSSSS];
			sss_jimenez_gauss_ver_pso_desc.vertex_shader_bytecode = Shader_Manager::s_vertex_shader_bytecode_buffers[(uint8_t)VShader_ID::FULL_QUAD];
			sss_jimenez_gauss_ver_pso_desc.depth_stencil_desc = Depth_Stencil_State_Desc::create();
			sss_jimenez_gauss_ver_pso_desc.rasterizer_desc = Rasterizer_State_Desc::create();
			sss_jimenez_gauss_ver_pso_desc.input_elements_desc = nullptr;
			sss_jimenez_gauss_ver_pso_desc.num_elements = 0;

			sss_jimenez_gauss_ver_pso_desc.depth_stencil_desc.depth_enable = false;
			sss_jimenez_gauss_ver_pso_desc.depth_stencil_desc.stencil_enable = true;
			sss_jimenez_gauss_ver_pso_desc.depth_stencil_desc.stencil_read_mask = D3D11_DEFAULT_STENCIL_READ_MASK;
			sss_jimenez_gauss_ver_pso_desc.depth_stencil_desc.stencil_write_mask = 0x00;
			sss_jimenez_gauss_ver_pso_desc.depth_stencil_desc.front_face.stencil_test_func = Comparison_Func::EQUAL;
			sss_jimenez_gauss_ver_pso_desc.depth_stencil_desc.front_face.stencil_fail_op = Stencil_Op::KEEP;
			sss_jimenez_gauss_ver_pso_desc.depth_stencil_desc.front_face.stencil_depth_pass_op = Stencil_Op::KEEP;

			sss_jimenez_gauss_ver_pso_desc.stencil_ref_value = 3;

			sss_jimenez_gauss_ver_pso_desc.blend_desc = D3D11_BLEND_DESC{};
			sss_jimenez_gauss_ver_pso_desc.blend_desc.AlphaToCoverageEnable = FALSE;
			sss_jimenez_gauss_ver_pso_desc.blend_desc.IndependentBlendEnable = FALSE;
			sss_jimenez_gauss_ver_pso_desc.blend_desc.RenderTarget[0].BlendEnable = false;
			sss_jimenez_gauss_ver_pso_desc.blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			zassert(rhi.create_pso(&m_pipeline_states[Pipeline_State::SSS_JIMENEZ_GAUSS_HOR], sss_jimenez_gauss_ver_pso_desc).value == S_OK);
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

	static D3D11_BLEND_DESC create_default_blend_desc()
	{
		D3D11_BLEND_DESC blend_desc;
		blend_desc.AlphaToCoverageEnable = false;
		blend_desc.IndependentBlendEnable = false;
		blend_desc.RenderTarget[0].BlendEnable = false;
		blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
		blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		blend_desc.RenderTarget[1].BlendEnable = false;
		blend_desc.RenderTarget[2].BlendEnable = false;
		blend_desc.RenderTarget[3].BlendEnable = false;
		blend_desc.RenderTarget[4].BlendEnable = false;
		blend_desc.RenderTarget[5].BlendEnable = false;
		blend_desc.RenderTarget[6].BlendEnable = false;
		blend_desc.RenderTarget[7].BlendEnable = false;

		return blend_desc;
	}

	static PSO_Desc create_default_gbuff_desc()
	{
		PSO_Desc pso_desc{};
		pso_desc.pixel_shader_bytecode = Pixel_Shader::s_pixel_shader_bytecode_buffers[(uint8_t)PShader_ID::STANDARD];
		pso_desc.vertex_shader_bytecode = Vertex_Shader::s_vertex_shader_bytecode_buffers[(uint8_t)VShader_ID::STANDARD];

		pso_desc.rasterizer_desc.cull_mode = Cull_Mode::BACK;
		pso_desc.rasterizer_desc.fill_mode = Fill_Mode::SOLID;
		pso_desc.rasterizer_desc.is_front_counter_clock_wise = false;
		pso_desc.rasterizer_desc.is_depth_clip_enabled = true;

		pso_desc.depth_stencil_desc.depth_enable = true;
		pso_desc.depth_stencil_desc.depth_write_mask = D3D11_DEPTH_WRITE_MASK_ALL;
		pso_desc.depth_stencil_desc.depth_test_func = Comparison_Func::GREATER;

		pso_desc.depth_stencil_desc.stencil_enable = true;
		pso_desc.depth_stencil_desc.stencil_read_mask = D3D11_DEFAULT_STENCIL_READ_MASK;
		pso_desc.depth_stencil_desc.stencil_write_mask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
		pso_desc.depth_stencil_desc.front_face.stencil_test_func = Comparison_Func::ALWAYS;
		pso_desc.depth_stencil_desc.front_face.stencil_pass_depth_fail_op = Stencil_Op::KEEP;
		pso_desc.depth_stencil_desc.front_face.stencil_fail_op = Stencil_Op::KEEP;
		pso_desc.depth_stencil_desc.front_face.stencil_depth_pass_op = Stencil_Op::REPLACE;

		pso_desc.depth_stencil_desc.back_face.stencil_test_func = Comparison_Func::ALWAYS;
		pso_desc.depth_stencil_desc.back_face.stencil_pass_depth_fail_op = Stencil_Op::KEEP;
		pso_desc.depth_stencil_desc.back_face.stencil_fail_op = Stencil_Op::KEEP;
		pso_desc.depth_stencil_desc.back_face.stencil_depth_pass_op = Stencil_Op::REPLACE;

		pso_desc.stencil_ref_value = 1;

		pso_desc.input_elements_desc = s_vertex_layout_desc;
		pso_desc.num_elements = sizeof(s_vertex_layout_desc) / sizeof(s_vertex_layout_desc[0]);

		pso_desc.blend_desc = create_default_blend_desc();

		return pso_desc;
	}
}