#include "Platform.h"
#include "renderer/backend/BufferCache.h"
#include "renderer/backend/rhi/RenderHardwareInterface.h"
#include "GraphicsContext.h"

#include <array>

zorya::GraphicsContext::GraphicsContext(ID3D11Device* device) : m_device(device)
{
	zassert(m_device != nullptr);
	HRESULT hr = m_device->CreateDeferredContext(0, &m_context);
	zassert(hr == S_OK);

	hr = m_context->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)&m_annot);
	zassert(hr == S_OK);
}

void zorya::GraphicsContext::set_pso(const PSO_Handle pso_hnd)
{
	auto pso = rhi.get_pso_pointer(pso_hnd);
	m_context->PSSetShader(pso->pixel_shader, nullptr, 0);
	m_context->VSSetShader(pso->vertex_shader, nullptr, 0);
	m_context->OMSetDepthStencilState(pso->ds_state, pso->stencil_ref_value);
	m_context->OMSetBlendState(pso->blend_state, NULL, D3D11_DEFAULT_SAMPLE_MASK);
	m_context->IASetInputLayout(pso->input_layout);
}

void zorya::GraphicsContext::set_blend_factor(const PSO_Handle pso_hnd, const float* blend_factor)
{
	auto pso = rhi.get_pso_pointer(pso_hnd);
	m_context->OMSetBlendState(pso->blend_state, blend_factor, D3D11_DEFAULT_SAMPLE_MASK);

}

void zorya::GraphicsContext::set_vs_constant_buff(Constant_Buffer_Handle* cb_hnd, uint32_t start_slot, uint32_t num_slots)
{
	ID3D11Buffer** constant_buffs = (ID3D11Buffer**)malloc(sizeof(ID3D11Buffer*) * num_slots);
	for (int i = 0; i < num_slots; i++)
	{
		constant_buffs[i] = { rhi.get_cb_pointer(*(cb_hnd + i)) };
	}

	m_context->VSSetConstantBuffers(start_slot, num_slots, constant_buffs);
	free(constant_buffs);
}

void zorya::GraphicsContext::set_ps_constant_buff(Constant_Buffer_Handle* cb_hnd, uint32_t start_slot, uint32_t num_slots)
{
	ID3D11Buffer** constant_buffs = (ID3D11Buffer**)malloc(sizeof(ID3D11Buffer*) * num_slots);
	for (int i = 0; i < num_slots; i++)
	{
		constant_buffs[i] = {rhi.get_cb_pointer(*(cb_hnd + i))};
	}
	
	m_context->PSSetConstantBuffers(start_slot, num_slots, constant_buffs);
	free(constant_buffs);
}

void zorya::GraphicsContext::set_ps_resource(Render_SRV_Handle* srv_hnd, uint32_t start_slot, uint32_t num_slots)
{
	ID3D11ShaderResourceView** srv_arr = (ID3D11ShaderResourceView**)malloc(sizeof(ID3D11ShaderResourceView*) * num_slots);
	for (int i = 0; i < num_slots; i++)
	{
		srv_arr[i] = rhi.get_srv_pointer(*(srv_hnd + i));
	}

	m_context->PSSetShaderResources(start_slot, num_slots, srv_arr);
	free(srv_arr);
}

void zorya::GraphicsContext::set_vertex_buff(Buffer_Handle_t buffer_hnd)
{
	ID3D11Buffer* vertex_buffers[] = {buffer_cache.get_vertex_buffer(buffer_hnd).buffer};

	//TODO: remove hardcoding for strides and offset
	static UINT strides[] = { sizeof(Vertex) };
	static UINT offsets[] = { 0 };
	m_context->IASetVertexBuffers(0, vertex_buffers[0] == nullptr ? 0 : 1, vertex_buffers, strides, offsets);
}

void zorya::GraphicsContext::set_index_buff(Buffer_Handle_t buffer_hnd)
{
	ID3D11Buffer* index_buffer = buffer_cache.get_index_buffer(buffer_hnd).buffer;

	//TODO: remove hardcoding for format and offset
	m_context->IASetIndexBuffer(index_buffer, DXGI_FORMAT_R16_UINT, 0);
}

void zorya::GraphicsContext::set_viewports(const Render_Rect* viewport, int32_t num_viewports)
{
	D3D11_VIEWPORT _viewport;
	_viewport.TopLeftX = viewport->top_left_x;
	_viewport.TopLeftY = viewport->top_left_y;
	_viewport.Width = viewport->width;
	_viewport.Height = viewport->height;
	_viewport.MinDepth = 0.0f;
	_viewport.MaxDepth = 1.0f;

	m_context->RSSetViewports(num_viewports, &_viewport);
}

void zorya::GraphicsContext::set_render_targets(const Render_RTV_Handle* rt_hnds, const Render_DSV_Handle ds_hnd, uint32_t num_views)
{
	ID3D11RenderTargetView** rt_views = (ID3D11RenderTargetView**)malloc(sizeof(ID3D11RenderTargetView*) * num_views);
	for (int i = 0; i < num_views; i++)
	{
		rt_views[i] = rhi.get_rtv_pointer(*(rt_hnds + i));
	}
	ID3D11DepthStencilView* ds_view = rhi.get_dsv_pointer(ds_hnd);

	m_context->OMSetRenderTargets(num_views, rt_views, ds_view);

	free(rt_views);
}

void zorya::GraphicsContext::set_pixel_shader(const Pixel_Shader* shader)
{
	m_context->PSSetShader(shader->m_shader, nullptr, 0);
}

void zorya::GraphicsContext::set_vertex_shader(const Vertex_Shader * shader)
{
	m_context->VSSetShader(shader->m_shader, nullptr, 0);
}

void zorya::GraphicsContext::update_subresource(const Constant_Buffer_Handle cb_hnd, void* data)
{
	ID3D11Buffer* cbuff = rhi.get_cb_pointer(cb_hnd);
	m_context->UpdateSubresource(cbuff, 0, nullptr, data, 0, 0);
}

void zorya::GraphicsContext::clear_dsv(const Render_DSV_Handle dsv_hnd, bool clear_depth, float depth_clear_val, bool clear_stencil, uint8_t stencilc_clear_val)
{
	ID3D11DepthStencilView* dsv = rhi.get_dsv_pointer(dsv_hnd);
	uint32_t clear_flags = (clear_depth ? D3D11_CLEAR_DEPTH : 0) | (clear_stencil ? D3D11_CLEAR_STENCIL : 0);
	m_context->ClearDepthStencilView(dsv, clear_flags, depth_clear_val, stencilc_clear_val);
}

void zorya::GraphicsContext::clear_rtv(const Render_RTV_Handle rtv_hnd, DirectX::XMFLOAT4& clear_color)
{
	ID3D11RenderTargetView* rtv = rhi.get_rtv_pointer(rtv_hnd);
	m_context->ClearRenderTargetView(rtv, &clear_color.x);
}

void zorya::GraphicsContext::get_ds_state(ID3D11DepthStencilState** ds_state)
{
	m_context->OMGetDepthStencilState(ds_state, nullptr);
}

void zorya::GraphicsContext::set_ds_state(ID3D11DepthStencilState* ds_state)
{
	m_context->OMSetDepthStencilState(ds_state, 0);
}

void zorya::GraphicsContext::update_subresource(Shader_Resource* resource)
{
	//TODO: fix; here for some reason assuming resource its always constant buffer
	m_context->UpdateSubresource((ID3D11Resource*)rhi.get_cb_pointer(resource->m_hnd_gpu_cb), 0, nullptr, resource->m_parameters->cb_start, 0, 0);
}

void zorya::GraphicsContext::set_ps_resource(Shader_Resource* resource)
{
	auto srv_pointer = rhi.get_srv_pointer(resource->m_hnd_gpu_srv);
	m_context->PSSetShaderResources(resource->m_bind_point, resource->m_bind_count, (ID3D11ShaderResourceView**)&srv_pointer);
}

void zorya::GraphicsContext::set_ps_constant_buff(Shader_Resource* resource)
{
	auto cb_pointer = rhi.get_cb_pointer(resource->m_hnd_gpu_cb);
	m_context->PSSetConstantBuffers(resource->m_bind_point, resource->m_bind_count, (ID3D11Buffer**)&cb_pointer);
}

//TODO: submesh handle shouldnt probably be here; its a higher level concept
void zorya::GraphicsContext::draw(Submesh_Handle_t submesh_hnd)
{
	m_context->Draw(submesh_hnd.num_vertices, 0);
}

void zorya::GraphicsContext::draw_cube()
{
	m_context->Draw(36, 0);
}

void zorya::GraphicsContext::draw_full_quad()
{
	m_context->Draw(4, 0);
}

void zorya::GraphicsContext::draw_indexed(Submesh_Handle_t submesh_hnd)
{
	m_context->DrawIndexed(submesh_hnd.num_indices, 0, 0);
}

ID3D11CommandList* zorya::GraphicsContext::finish_command_list()
{
	HRESULT hr = m_context->FinishCommandList(false, &m_command_list);
	zassert(hr == S_OK);

	return m_command_list;
}

zorya::Render_Command_List::Render_Command_List(DX11_Render_Device& render_device) : gfx_context(render_device.m_device) {}

void zorya::Render_Command_List::draw_indexed(PSO_Handle _pso_hnd, Shader_Render_Targets&& shader_render_targets, Shader_Arguments&& shader_arguments, Buffer_Handle_t _hnd_index_buffer, const Buffer_Handle_t* _hnd_vert_buffers, uint32_t num_vert_buffers, D3D_PRIMITIVE_TOPOLOGY primitive_topology, Submesh_Handle_t hnd_submesh)
{
	Render_Command_Draw_Indexed* cmd = new Render_Command_Draw_Indexed;
	cmd->pso_hnd = _pso_hnd;
	
	cmd->shader_render_targets = shader_render_targets;

	uint32_t size_arr_vert_buffers = num_vert_buffers * sizeof(Buffer_Handle_t);
	cmd->hnd_vertex_buffers = (Buffer_Handle_t*)malloc(size_arr_vert_buffers);
	memcpy(cmd->hnd_vertex_buffers, _hnd_vert_buffers, size_arr_vert_buffers);
	cmd->num_vertex_buffers = num_vert_buffers;

	cmd->hnd_index_buffer = _hnd_index_buffer;

	cmd->shader_arguments = shader_arguments;

	cmd->primitive_topology = primitive_topology;
	cmd->hnd_submesh = hnd_submesh;

	cmd_list.push_back(cmd);
}

void zorya::Render_Command_List::update_buffer(Arena& arena, Constant_Buffer_Handle hnd_cb, void* new_data, size_t size_in_bytes)
{
	auto cmd = new Render_Command_Update_Buffer;
	cmd->hnd_cb = hnd_cb;
	cmd->new_data = arena.alloc(size_in_bytes, 256);
	memcpy_s(cmd->new_data, size_in_bytes, new_data, size_in_bytes);
	cmd_list.push_back(cmd);
}

void zorya::Render_Command_List::copy_texture(Render_RTV_Handle dest, Render_RTV_Handle src)
{
	auto cmd = new Render_Command_Copy_Texture;
	cmd->src_tex = src;
	cmd->dest_tex = dest;

	cmd_list.push_back(cmd);
}

void zorya::Render_Command_List::begin_event(const wchar_t* event_name)
{
	auto cmd = new Render_Command_Begin_Event;
	cmd->event_name = event_name;
	cmd_list.push_back(cmd);
}

void zorya::Render_Command_List::end_event()
{
	auto cmd = new Render_Command_End_Event;
	cmd_list.push_back(cmd);
}

void zorya::Render_Command_List::set_viewport(Render_Rect viewport)
{
	auto cmd = new Render_Command_Set_Viewport;
	cmd->viewport = viewport;

	cmd_list.push_back(cmd);
}

void zorya::Render_Command_List::clear_rtv(Render_RTV_Handle rt_hnd, float* clear_color)
{
	if (rt_hnd.index > 0)
	{
		auto cmd = new Render_Command_Clear_Render_Target;
		cmd->rt_hnd = rt_hnd;
		cmd->clear_color[0] = clear_color[0];
		cmd->clear_color[1] = clear_color[1];
		cmd->clear_color[2] = clear_color[2];
		cmd->clear_color[3] = clear_color[3];

		cmd_list.push_back(cmd);
	}
}

void zorya::Render_Command_List::clear_dsv(const Render_DSV_Handle dsv_hnd, bool clear_depth, float depth_clear_val, bool clear_stencil, uint8_t stencil_clear_val)
{
	if (dsv_hnd.index > 0)
	{
		auto cmd = new Render_Command_Clear_DSV;

		cmd->dsv_hnd = dsv_hnd;
		cmd->clear_depth = clear_depth;
		cmd->clear_stencil = clear_stencil;
		cmd->depth_clear_val = depth_clear_val;
		cmd->stencil_clear_val = stencil_clear_val;

		cmd_list.push_back(cmd);
	}
}


void zorya::Render_Command_List::draw(PSO_Handle _pso_hnd, Shader_Render_Targets&& shader_render_targets, Shader_Arguments&& shader_arguments, const Buffer_Handle_t* _hnd_vert_buffers, uint32_t num_vert_buffers, D3D_PRIMITIVE_TOPOLOGY primitive_topology, Submesh_Handle_t hnd_submesh, float* blend_factor)
{
	Render_Command_Draw* cmd = new Render_Command_Draw;
	cmd->pso_hnd = _pso_hnd;
	if (blend_factor == nullptr) for (int i = 0; i < 4; i++) cmd->blend_factor[i] = 1.0f;
	else memcpy(&cmd->blend_factor[0], blend_factor, sizeof(cmd->blend_factor));

	cmd->shader_render_targets = shader_render_targets;

	uint32_t size_arr_vert_buffers = num_vert_buffers * sizeof(Buffer_Handle_t);
	cmd->hnd_vertex_buffers = (Buffer_Handle_t*)malloc(size_arr_vert_buffers);
	memcpy(cmd->hnd_vertex_buffers, _hnd_vert_buffers, size_arr_vert_buffers);
	cmd->num_vertex_buffers = num_vert_buffers;

	cmd->shader_arguments = shader_arguments;

	cmd->primitive_topology = primitive_topology;
	cmd->hnd_submesh = hnd_submesh;

	cmd_list.push_back(cmd);
}

void zorya::Render_Command_List::clear()
{
	for (auto cmd : cmd_list)
	{
		if(cmd != nullptr) free(cmd);
	}

	cmd_list.clear();
	gfx_context.m_command_list->Release();
	gfx_context.m_command_list = NULL;
}

ID3D11CommandList* zorya::Render_Command_List::finish_command_list()
{
	Render_RTV_Handle null_rtvs[8] = { 0 };
	Render_SRV_Handle null_srvs[10] = { 0 };

	for (auto cmd : cmd_list)
	{
		switch (cmd->type)
		{
		case zorya::Render_Command_Type::DRAW:
		{
			auto _cmd = static_cast<Render_Command_Draw*>(cmd);

			auto& shader_render_targets = _cmd->shader_render_targets;
			gfx_context.set_render_targets(null_rtvs, Render_DSV_Handle{ 0 }, 8);
			gfx_context.set_render_targets(shader_render_targets.rtv_hnds, shader_render_targets.dsv_hnd, shader_render_targets.num_rtv_hnds);

			gfx_context.set_pso(_cmd->pso_hnd);
			gfx_context.set_blend_factor(_cmd->pso_hnd, _cmd->blend_factor);
			gfx_context.m_context->IASetPrimitiveTopology(_cmd->primitive_topology);
			//TODO: fix; should allow multiple vertex buffers
			gfx_context.set_vertex_buff(_cmd->num_vertex_buffers > 0 ? _cmd->hnd_vertex_buffers[0] : Buffer_Handle_t{0});

			auto& shader_args = _cmd->shader_arguments;
			gfx_context.set_vs_constant_buff(shader_args.vs_cb_handles, 0, shader_args.num_vs_cb_handles);
			gfx_context.set_ps_constant_buff(shader_args.ps_cb_handles, 0, shader_args.num_ps_cb_handles);
			gfx_context.set_ps_resource(null_srvs, 0, 10);
			gfx_context.set_ps_resource(shader_args.ps_srv_handles, 0, shader_args.num_ps_srv_handles);

			gfx_context.draw(_cmd->hnd_submesh);

			break;
		}
		case zorya::Render_Command_Type::DRAW_INDEXED:
		{
			auto _cmd = static_cast<Render_Command_Draw_Indexed*>(cmd);

			auto& shader_render_targets = _cmd->shader_render_targets;
			gfx_context.set_render_targets(null_rtvs, Render_DSV_Handle{ 0 }, 8);
			gfx_context.set_render_targets(shader_render_targets.rtv_hnds, shader_render_targets.dsv_hnd, shader_render_targets.num_rtv_hnds);

			gfx_context.set_pso(_cmd->pso_hnd);
			gfx_context.m_context->IASetPrimitiveTopology(_cmd->primitive_topology);
			//TODO: implement setting multiple vertex buffers
			gfx_context.set_vertex_buff(_cmd->hnd_vertex_buffers[0]);
			gfx_context.set_index_buff(_cmd->hnd_index_buffer);
			
			auto& shader_args = _cmd->shader_arguments;
			gfx_context.set_vs_constant_buff(shader_args.vs_cb_handles, 0, shader_args.num_vs_cb_handles);
			gfx_context.set_ps_constant_buff(shader_args.ps_cb_handles, 0, shader_args.num_ps_cb_handles);
			gfx_context.set_ps_resource(null_srvs, 0, 10);
			gfx_context.set_ps_resource(shader_args.ps_srv_handles, 0, shader_args.num_ps_srv_handles);

			gfx_context.draw_indexed(_cmd->hnd_submesh);

			break;
		}
		case zorya::Render_Command_Type::UPDATE_BUFFER:
		{
			auto _cmd = static_cast<Render_Command_Update_Buffer*>(cmd);

			gfx_context.update_subresource(_cmd->hnd_cb, _cmd->new_data);

			break;
		}
		case zorya::Render_Command_Type::COPY_TEXTURE:
		{
			auto _cmd = static_cast<Render_Command_Copy_Texture*>(cmd);

			//TODO: fix everything
			auto src = rhi.get_rtv_pointer(_cmd->src_tex);
			auto dest = rhi.get_rtv_pointer(_cmd->dest_tex);

			ID3D11Resource *_dest, *_src;
			src->GetResource(&_src);
			dest->GetResource(&_dest);

			//gfx_context.m_context->CopySubresourceRegion(_dest, 0, 0, 0, 0, _src, 0, nullptr);
			gfx_context.m_context->CopyResource(_dest, _src);

			break;
		}
		case zorya::Render_Command_Type::BEGIN_EVENT:
		{
			auto _cmd = static_cast<Render_Command_Begin_Event*>(cmd);

			gfx_context.m_annot->BeginEvent(_cmd->event_name);

			break;
		}
		case zorya::Render_Command_Type::END_EVENT:
		{
			gfx_context.m_annot->EndEvent();

			break;
		}
		case zorya::Render_Command_Type::SET_VIEWPORT:
		{
			auto _cmd = static_cast<Render_Command_Set_Viewport*>(cmd);
			gfx_context.set_viewports(&_cmd->viewport, 1);

			break;
		}
		case zorya::Render_Command_Type::CLEAR_RENDER_TARGET:
		{
			auto _cmd = static_cast<Render_Command_Clear_Render_Target*>(cmd);
			auto clear_col = DirectX::XMFLOAT4(_cmd->clear_color);
			gfx_context.clear_rtv(_cmd->rt_hnd, clear_col);

			break;
		}
		case zorya::Render_Command_Type::CLEAR_DSV:
		{
			auto _cmd = static_cast<Render_Command_Clear_DSV*>(cmd);
			gfx_context.clear_dsv(_cmd->dsv_hnd, _cmd->clear_depth, _cmd->depth_clear_val, _cmd->clear_stencil, _cmd->stencil_clear_val);

			break;
		}
		
		default:
			break;
		}
	}

	return gfx_context.finish_command_list();
}

zorya::Shader_Render_Targets zorya::create_shader_render_targets(Arena& arena, Render_RTV_Handle* rtv_hnds, uint32_t num_rtv_hnds, Render_DSV_Handle dsv_hnd)
{
	zorya::Shader_Render_Targets shader_render_targets;

	uint32_t arr_size = num_rtv_hnds * sizeof(Render_RTV_Handle);
	shader_render_targets.rtv_hnds = (Render_RTV_Handle*)arena.alloc(arr_size, alignof(Render_RTV_Handle));
	memcpy_s(shader_render_targets.rtv_hnds, arr_size, rtv_hnds, arr_size);
	shader_render_targets.num_rtv_hnds = num_rtv_hnds;

	shader_render_targets.dsv_hnd = dsv_hnd;

	return shader_render_targets;
}
