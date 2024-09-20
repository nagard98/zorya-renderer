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
#include "xxhash.h"

namespace zorya
{

	Shader_Arguments create_shader_arguments(Arena& arena, Constant_Buffer_Handle* arr_vs_cb_hnds, uint32_t num_vs_cb_hnds, Render_SRV_Handle* arr_vs_srv_hnds, uint32_t num_vs_srv_hnds, Constant_Buffer_Handle* arr_ps_cb_hnds, uint32_t num_ps_cb_hnds, Render_SRV_Handle* arr_ps_srv_hnds, uint32_t num_ps_srv_hnds)
	{
		Shader_Arguments shader_arguments;

		{
			uint32_t cb_arr_size = num_vs_cb_hnds * sizeof(Constant_Buffer_Handle);
			//TODO: are there allignment problems?
			shader_arguments.vs_cb_handles = (Constant_Buffer_Handle*)arena.alloc(cb_arr_size, alignof(Constant_Buffer_Handle));
			shader_arguments.num_vs_cb_handles = num_vs_cb_hnds;
			memcpy(shader_arguments.vs_cb_handles, arr_vs_cb_hnds, cb_arr_size);
		}

		{
			uint32_t srv_arr_size = num_vs_srv_hnds * sizeof(Render_SRV_Handle);
			shader_arguments.vs_srv_handles = (Render_SRV_Handle*)arena.alloc(srv_arr_size, alignof(Render_SRV_Handle));
			shader_arguments.num_vs_srv_handles = num_vs_srv_hnds;
			memcpy(shader_arguments.vs_srv_handles, arr_vs_srv_hnds, srv_arr_size);
		}

		{
			uint32_t cb_ps_arr_size = num_ps_cb_hnds * sizeof(Constant_Buffer_Handle);
			//TODO: are there allignment problems?
			shader_arguments.ps_cb_handles = (Constant_Buffer_Handle*)arena.alloc(cb_ps_arr_size, alignof(Constant_Buffer_Handle));
			shader_arguments.num_ps_cb_handles = num_ps_cb_hnds;
			memcpy(shader_arguments.ps_cb_handles, arr_ps_cb_hnds, cb_ps_arr_size);
		}

		{
			uint32_t srv_ps_arr_size = num_ps_srv_hnds * sizeof(Render_SRV_Handle);
			shader_arguments.ps_srv_handles = (Render_SRV_Handle*)arena.alloc(srv_ps_arr_size, alignof(Render_SRV_Handle));
			shader_arguments.num_ps_srv_handles = num_ps_srv_hnds;
			memcpy(shader_arguments.ps_srv_handles, arr_ps_srv_hnds, srv_ps_arr_size);
		}

		return shader_arguments;
	}

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
		load_shader_bytecode("./shaders/Present.cso"),//ShaderBytecode{ g_Present, sizeof(g_Present) }
		load_shader_bytecode("./shaders/Composit.cso"),
		load_shader_bytecode("./shaders/ShadowMaskBuild.cso")
	};

	Shader_Bytecode Shader_Manager::s_pixel_shader_bytecode_buffers[] = {
		Shader_Bytecode{},
		load_shader_bytecode("./shaders/GBufferPS.cso"), //ShaderBytecode{ g_GBufferPS, sizeof(g_GBufferPS) },
		load_shader_bytecode("./shaders/SkyboxPS.cso"), //ShaderBytecode{ g_SkyboxPS, sizeof(g_SkyboxPS) },
		Shader_Bytecode{},
		load_shader_bytecode("./shaders/SSSSS.cso"), //ShaderBytecode{ g_SSSSS, sizeof(g_SSSSS) },
		load_shader_bytecode("./shaders/Lighting.cso"), //ShaderBytecode{ g_Lighting, sizeof(g_Lighting) },
		load_shader_bytecode("./shaders/ShadowMapping.cso"), //ShaderBytecode{ g_ShadowMapping, sizeof(g_ShadowMapping) },
		load_shader_bytecode("./shaders/Present.cso"), //ShaderBytecode{ g_Present, sizeof(g_Present) }
		load_shader_bytecode("./shaders/Composit.cso"),
		load_shader_bytecode("./shaders/ShadowMaskBuild.cso")
	};

	Pixel_Shader Pixel_Shader::create(const BYTE* shader_bytecode, size_t bytecode_size)
	{
		ID3D11PixelShader* shader = nullptr;
		HRESULT h_res = rhi.m_device.m_device->CreatePixelShader(shader_bytecode, bytecode_size, nullptr, &shader);

		ID3D11ShaderReflection* shader_reflection = nullptr;
		h_res = D3DReflect(shader_bytecode, bytecode_size, IID_ID3D11ShaderReflection, (void**)&shader_reflection);

		D3D11_SHADER_DESC shader_desc;
		shader_reflection->GetDesc(&shader_desc);
		int i = 0;

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

	Resource_Type to_zry_resource_type(_D3D_SHADER_INPUT_TYPE d3d_resource_type) {
		switch (d3d_resource_type){
		
		case D3D_SIT_CBUFFER:
			return ZRY_CBUFF;

		case D3D_SIT_TEXTURE:
			return ZRY_TEX;

		default:
			return ZRY_UNSUPPORTED;
		}
	}

	CB_Var_Desc to_zry_variable_type(D3D11_SHADER_TYPE_DESC d3d_var_desc) {
		CB_Var_Desc zry_var_desc;
		
		switch (d3d_var_desc.Type)
		{
		case (D3D_SVT_FLOAT):
		{
			zry_var_desc.variable_type = VAR_REFL_TYPE::FLOAT;
			zry_var_desc.columns = d3d_var_desc.Columns;
			zry_var_desc.elements = 0;
			break;
		}
		case (D3D_SVT_BOOL):
		{
			zry_var_desc.variable_type = VAR_REFL_TYPE::BOOL;
			zry_var_desc.columns = d3d_var_desc.Columns;
			zry_var_desc.elements = 0;
			break;
		}
		case (D3D_SVT_UINT):
		{
			zry_var_desc.variable_type = VAR_REFL_TYPE::UINT32;
			zry_var_desc.columns = d3d_var_desc.Columns;
			zry_var_desc.elements = 0;
			break;
		}
		default:
		{
			zry_var_desc.variable_type = VAR_REFL_TYPE::NOT_SUPPORTED;
			break;
		}

		}

		return zry_var_desc;
	}

	std::vector<Shader_Resource> Pixel_Shader::build_uniforms_struct()
	{
		D3D11_SHADER_DESC shader_desc;
		HRESULT hr = m_shader_reflection->GetDesc(&shader_desc);

		if (FAILED(hr))
		{
			return std::vector<Shader_Resource>{};
		} else
		{
			int bound_resources = shader_desc.BoundResources;
			std::vector<Shader_Resource> shader_resources;
			shader_resources.reserve(bound_resources);

			D3D11_SHADER_INPUT_BIND_DESC desc;
			for (int i = 0; i < bound_resources; i++)
			{
				m_shader_reflection->GetResourceBindingDesc(i, &desc);
				Resource_Type type = to_zry_resource_type(desc.Type);
				
				switch (type)
				{

				case Resource_Type::ZRY_CBUFF:
				{
					ID3D11ShaderReflectionConstantBuffer* cbuff = m_shader_reflection->GetConstantBufferByName(desc.Name);

					//ID3D11Buffer* cb_resource = nullptr;
					Constant_Buffer_Handle hnd_cb_resource{ 0 };
					Constant_Buffer_Data* cb_data = new Constant_Buffer_Data;
					

					D3D11_SHADER_BUFFER_DESC cbuff_desc;
					HRESULT hr = cbuff->GetDesc(&cbuff_desc);
					
					if (!FAILED(hr))
					{

						//TODO: make alligned malloc for constant buffers as required by dx11
						cb_data->cb_start = malloc(cbuff_desc.Size);
						ZeroMemory(cb_data->cb_start, cbuff_desc.Size);
						cb_data->variables = new CB_Variable[cbuff_desc.Variables];
						cb_data->num_variables = cbuff_desc.Variables;

						D3D11_BUFFER_DESC cb_res_desc{};
						cb_res_desc.ByteWidth = cbuff_desc.Size;
						cb_res_desc.Usage = D3D11_USAGE_DEFAULT;
						cb_res_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
						
						for (int i = 0; i < cbuff_desc.Variables; i++)
						{
							ID3D11ShaderReflectionVariable* refl_cbuff_var = cbuff->GetVariableByIndex(i);
							D3D11_SHADER_VARIABLE_DESC var_desc;
							hr = refl_cbuff_var->GetDesc(&var_desc);
							if (!FAILED(hr))
							{
								CB_Variable* variable = &cb_data->variables[i];
								variable->offset_in_bytes = var_desc.StartOffset;
								variable->size_in_bytes = var_desc.Size;

								if (var_desc.DefaultValue != nullptr)
								{
									unsigned char* pointer_to_var_in_struct = static_cast<unsigned char*>(cb_data->cb_start) + var_desc.StartOffset;
									memcpy_s(pointer_to_var_in_struct, var_desc.Size, var_desc.DefaultValue, var_desc.Size);
								}

								int name_buff_len = strnlen_s(var_desc.Name, 255) + 1;
								variable->name = (char*)malloc(name_buff_len);
								strncpy_s(variable->name, name_buff_len, var_desc.Name, name_buff_len);
								

								ID3D11ShaderReflectionType* refl_type = refl_cbuff_var->GetType();
								D3D11_SHADER_TYPE_DESC var_type_desc;
								refl_type->GetDesc(&var_type_desc);

								variable->description = to_zry_variable_type(var_type_desc);

							}
						}

						D3D11_SUBRESOURCE_DATA cb_subres_data{};
						cb_subres_data.pSysMem = cb_data->cb_start;
						
						//hr = rhi.m_device.m_device->CreateBuffer(&cb_res_desc, &cb_subres_data, &cb_resource);
						hr = rhi.create_constant_buffer(&hnd_cb_resource, &cb_res_desc).value;
						//TODO: fix; don't use immediate context
						rhi.m_context->UpdateSubresource(rhi.get_cb_pointer(hnd_cb_resource), 0, nullptr, cb_data->cb_start, 0, 0);

					}
					shader_resources.emplace_back(desc.Name, desc.BindPoint, desc.BindCount, to_zry_resource_type(desc.Type), Render_Resource_Handle{ hnd_cb_resource.index }, cb_data);
					break;
				}

				default:
				{
					shader_resources.emplace_back(desc.Name, desc.BindPoint, desc.BindCount, to_zry_resource_type(desc.Type), Render_Resource_Handle{ 0 }, nullptr);
					break;
				}

				}
			}

			return shader_resources;
		}
	}

	void Pixel_Shader::free_shader()
	{
		if (m_shader)m_shader->Release();
		if (m_shader_reflection) m_shader_reflection->Release();
		//TODO: IMPORTANT! Add removal from registered shaders (in shader struct add its id, to know where to remove)
	}

	HRESULT Pixel_Shader::bind_texture_2d(const char* binding_name, Shader_Texture2D** const texture)
	{
		D3D11_SHADER_INPUT_BIND_DESC bind_desc;
		HRESULT hr = m_shader_reflection->GetResourceBindingDescByName(binding_name, &bind_desc);
		RETURN_IF_FAILED(hr);

		rhi.m_gfx_context->m_context->PSSetShaderResources(bind_desc.BindPoint, 1, texture);

		return S_OK;
	}

	HRESULT Pixel_Shader::bind_constant_buffer(const char* binding_name, ID3D11Buffer* const constant_buffer)
	{
		D3D11_SHADER_INPUT_BIND_DESC bind_desc;
		HRESULT hr = m_shader_reflection->GetResourceBindingDescByName(binding_name, &bind_desc);
		RETURN_IF_FAILED(hr);

		rhi.m_gfx_context->m_context->PSSetConstantBuffers(bind_desc.BindPoint, 1, &constant_buffer);

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

		rhi.m_gfx_context->m_context->PSSetConstantBuffers(bind_desc.BindPoint, 1, &constant_buffer);

		return hr;
	}

	ID3D11VertexShader* Vertex_Shader::s_registered_vertex_shaders[] = { nullptr };
	Shader_Bytecode Vertex_Shader::s_vertex_shader_bytecode_buffers[] = {
		load_shader_bytecode("./shaders/GBufferVS.cso"), //ShaderBytecode{ g_GBufferVS, sizeof(g_GBufferVS) },
		load_shader_bytecode("./shaders/SkyboxVS.cso"), //ShaderBytecode{ g_SkyboxVS, sizeof(g_SkyboxVS) },
		load_shader_bytecode("./shaders/DepthVS.cso"), //ShaderBytecode{ g_DepthVS, sizeof(g_DepthVS) },
		load_shader_bytecode("./shaders/FullscreenQuad.cso"), //ShaderBytecode{ g_FullscreenQuad, sizeof(g_FullscreenQuad) }
	};

	Shader_Bytecode Shader_Manager::s_vertex_shader_bytecode_buffers[] = {
		load_shader_bytecode("./shaders/GBufferVS.cso"), //ShaderBytecode{ g_GBufferVS, sizeof(g_GBufferVS) },
		load_shader_bytecode("./shaders/SkyboxVS.cso"), //ShaderBytecode{ g_SkyboxVS, sizeof(g_SkyboxVS) },
		load_shader_bytecode("./shaders/DepthVS.cso"), //ShaderBytecode{ g_DepthVS, sizeof(g_DepthVS) },
		load_shader_bytecode("./shaders/FullscreenQuad.cso"), //ShaderBytecode{ g_FullscreenQuad, sizeof(g_FullscreenQuad) }
	};


	Shader_Resource::Shader_Resource(const char* name, uint8_t bind_point, uint8_t bind_count, Resource_Type type, Render_Resource_Handle hnd_resource, void* referenced_resource)
	{
		int name_buff_len = strnlen_s(name, 255) + 1;
		m_name = (char*)malloc(name_buff_len);
		strncpy_s(m_name, name_buff_len, name, name_buff_len);

		m_bind_point = bind_point;
		m_bind_count = bind_count;
		m_type = type;
		//m_hnd_resource = 0;
		//m_fake_hnd_resource = fake_hnd_resource;


		switch (type)
		{
		case zorya::ZRY_TEX:
		{
			m_texture = static_cast<Texture2D*>(referenced_resource);
			m_hnd_gpu_srv = { hnd_resource.index };
			break;
		}
		case zorya::ZRY_CBUFF:
		{
			m_parameters = static_cast<Constant_Buffer_Data*>(referenced_resource);
			m_hnd_gpu_cb = { hnd_resource.index };
			break;
		}
		case zorya::ZRY_UNSUPPORTED:
		{
			m_texture = nullptr;
			break;
		}
		default:
		{
			m_texture = nullptr;
			break;
		}
		}

	}

	bool set_constant_buff_var(Constant_Buffer_Data* cb_data, const char* var_name, void* value, size_t val_size_in_bytes)
	{
		bool is_op_success = false;
		if (cb_data != nullptr)
		{
			for (int i = 0; i < cb_data->num_variables; i++)
			{
				CB_Variable* variable = &cb_data->variables[i];

				if (strncmp(variable->name, var_name, MAX_PATH) == 0)
				{
					char* pointer_to_var_in_struct = static_cast<char*>(cb_data->cb_start) + variable->offset_in_bytes;
					if (memcpy_s(pointer_to_var_in_struct, variable->size_in_bytes, value, val_size_in_bytes) == 0)
					{
						is_op_success = true;
						break;
					}
				}

			}
		}

		return is_op_success;
	}
	
	Shader_Arguments create_shader_arguments(Arena& arena, const std::vector<Shader_Resource>& shader_resources)
	{
		for (auto resource : shader_resources)
		{

		}

		return Shader_Arguments{};
	}

	//Shader_Arguments create_shader_arguments(Arena& arena, constant_buffer_handle2* arr_cb_hnds, uint32_t num_cb_hnds, Render_SRV_Handle* arr_srv_hnds, uint32_t num_srv_hnds)
	//{
	//	Shader_Arguments shader_arguments;
	//	uint32_t cb_arr_size = num_cb_hnds * sizeof(constant_buffer_handle2);
	//	//TODO: are there allignment problems
	//	shader_arguments.cb_handles = (constant_buffer_handle2*)arena.alloc(cb_arr_size, alignof(constant_buffer_handle2));
	//	shader_arguments.num_cb_handles = num_cb_hnds;
	//	memcpy(shader_arguments.cb_handles, arr_cb_hnds, cb_arr_size);

	//	uint32_t srv_arr_size = num_srv_hnds * sizeof(Render_SRV_Handle);
	//	shader_arguments.srv_handles = (Render_SRV_Handle*)arena.alloc(srv_arr_size, alignof(Render_SRV_Handle));
	//	shader_arguments.num_srv_handles = num_srv_hnds;
	//	memcpy(shader_arguments.srv_handles, arr_srv_hnds, srv_arr_size);

	//	return shader_arguments;
	//}

	bool set_shader_resource_asset(std::vector<Shader_Resource>& resources, const char* resource_name, Texture2D* tex_asset, const Texture_Import_Config* tex_imp_conf)
	{
		bool is_op_success = false;
		
		for (auto& resource : resources)
		{
			if ((resource.m_type == Resource_Type::ZRY_TEX) && (strncmp(resource.m_name, resource_name, MAX_PATH) == 0))
			{
				resource.m_texture = tex_asset;
				if (tex_asset != nullptr)
				{
					tex_asset->load_asset(tex_imp_conf);
					//TODO: pass correct value for is_srgb
					RHI_RESULT res = rhi.load_texture2(tex_asset, tex_imp_conf, &resource.m_hnd_gpu_srv);
					assert(res == S_OK);
					if (res != S_OK) return false;
				}

				break;
			}
		}

		return is_op_success;
	}

	bool get_constant_buff_var(Constant_Buffer_Data* cb_data, const char* var_name, void* value)
	{
		bool is_op_success = false;

		if (cb_data != nullptr)
		{
			for (int i = 0; i < cb_data->num_variables; i++)
			{
				CB_Variable* variable = &cb_data->variables[i];

				if (strncmp(variable->name, var_name, MAX_PATH) == 0)
				{
					char* pointer_to_var_in_struct = static_cast<char*>(cb_data->cb_start) + variable->offset_in_bytes;
					//TODO: fix memory size of value
					if (memcpy_s(value, sizeof(uint32_t), pointer_to_var_in_struct, variable->size_in_bytes) == 0)
					{
						is_op_success = true;
						break;
					}
				}

			}
		}

		return is_op_success;
	}

	Shader_Manager::Shader_Manager()
	{
		ps_handles.insert({ XXH64(nullptr, 0, 0), Pixel_Shader_Handle{0} });
		vs_handles.insert({ XXH64(nullptr, 0, 0), Vertex_Shader_Handle{0} });
	}
	
	Pixel_Shader_Handle Shader_Manager::ps_hnd_from_bytecode(const Shader_Bytecode& bytecode)
	{
		uint64_t ps_hash = XXH64(bytecode.bytecode, bytecode.size_in_bytes, 0);
		auto found_it = ps_handles.find(ps_hash);
		Pixel_Shader_Handle ps_hnd{ 0 };

		if (found_it == ps_handles.end())
		{
			auto err = rhi.create_pixel_shader(&ps_hnd, bytecode);
			zassert(err.value == S_OK);
			ps_handles.insert({ ps_hash, ps_hnd });
		}
		else
		{
			ps_hnd = found_it->second;
		}

		return ps_hnd;
	}

	Vertex_Shader_Handle Shader_Manager::vs_hnd_from_bytecode(const Shader_Bytecode& bytecode)
	{
		uint64_t vs_hash = XXH64(bytecode.bytecode, bytecode.size_in_bytes, 0);
		auto found_it = vs_handles.find(vs_hash);
		Vertex_Shader_Handle vs_hnd{ 0 };

		if (found_it == vs_handles.end())
		{
			auto err = rhi.create_vertex_shader(&vs_hnd, bytecode);
			zassert(err.value == S_OK);
			vs_handles.insert({ vs_hash, vs_hnd });
		} else
		{
			vs_hnd = found_it->second;
		}

		return vs_hnd;
	}

	Shader_Manager shader_manager;
}