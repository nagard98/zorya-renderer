#ifndef GRAPHICS_CONTEXT_H_
#define GRAPHICS_CONTEXT_H_

#include <d3d11.h>
#include <DirectXMath.h>
#include "utils/Arena.h"
#include "Platform.h"

#include "BufferCache.h"
#include "ConstantBuffer.h"
#include "renderer/frontend/Shader.h"
#include "renderer/backend/rhi/RenderDevice.h"
#include "renderer/frontend/Shader.h"
#include "renderer/backend/PipelineStateObject.h"

namespace zorya
{
	
	struct Render_Rect
	{
		u32 top_left_x;
		u32 top_left_y;
		u32 width;
		u32 height;
	};

	struct Render_Box
	{
		u32 top_left_x;
		u32 top_left_y;
		u32 top_left_z;
		u32 width;
		u32 height;
		u32 depth;
	};


	struct Shader_Render_Targets
	{
		Render_RTV_Handle* rtv_hnds;
		uint32_t num_rtv_hnds;

		Render_DSV_Handle dsv_hnd;
	};

	Shader_Render_Targets create_shader_render_targets(Arena& arena, Render_RTV_Handle* rtv_hnds, uint32_t num_rtv_hnds, Render_DSV_Handle dsv_hnd);

	enum class Render_Command_Type
	{
		DRAW,
		DRAW_INDEXED,
		UPDATE_TEXTURE,
		UPDATE_BUFFER,
		COPY_TEXTURE,
		COPY_BUFFER,
		BEGIN_EVENT,
		END_EVENT,
		SET_VIEWPORT,
		CLEAR_RENDER_TARGET,
		CLEAR_DSV
	};

	struct Render_Command
	{
		Render_Command_Type type;
	};

	template <Render_Command_Type TYPE>
	struct Render_Command_Typed : Render_Command
	{
		static const Render_Command_Type Type = TYPE;

		Render_Command_Typed() { type = Type; }
	};

	struct Render_Command_Draw_Indexed : Render_Command_Typed<Render_Command_Type::DRAW_INDEXED>
	{
		Shader_Render_Targets shader_render_targets;

		PSO_Handle pso_hnd;
		Shader_Arguments shader_arguments;

		Buffer_Handle_t hnd_index_buffer;
		Buffer_Handle_t* hnd_vertex_buffers;
		uint32_t num_vertex_buffers;

		D3D_PRIMITIVE_TOPOLOGY primitive_topology;

		Submesh_Handle_t hnd_submesh;
	};
	
	struct Render_Command_Draw : Render_Command_Typed<Render_Command_Type::DRAW>
	{
		Shader_Render_Targets shader_render_targets;

		PSO_Handle pso_hnd;
		Shader_Arguments shader_arguments;

		Buffer_Handle_t* hnd_vertex_buffers;
		uint32_t num_vertex_buffers;

		D3D_PRIMITIVE_TOPOLOGY primitive_topology;

		Submesh_Handle_t hnd_submesh;
	};

	struct Render_Command_Copy_Texture : Render_Command_Typed<Render_Command_Type::COPY_TEXTURE>
	{
		Render_RTV_Handle src_tex;
		Render_RTV_Handle dest_tex;
	};

	struct Render_Command_Update_Buffer : Render_Command_Typed<Render_Command_Type::UPDATE_BUFFER>
	{
		Constant_Buffer_Handle hnd_cb;
		void* new_data;
	};

	struct Render_Command_Begin_Event : Render_Command_Typed<Render_Command_Type::BEGIN_EVENT>
	{
		const wchar_t* event_name;
	};

	struct Render_Command_End_Event : Render_Command_Typed<Render_Command_Type::END_EVENT>{};

	struct Render_Command_Set_Viewport : Render_Command_Typed<Render_Command_Type::SET_VIEWPORT>
	{
		Render_Rect viewport;
	};

	struct Render_Command_Clear_Render_Target : Render_Command_Typed<Render_Command_Type::CLEAR_RENDER_TARGET>
	{
		Render_RTV_Handle rt_hnd;
		float clear_color[4];
	};

	struct Render_Command_Clear_DSV : Render_Command_Typed<Render_Command_Type::CLEAR_DSV>
	{
		Render_DSV_Handle dsv_hnd;
		float depth_clear_val;
		uint8_t stencil_clear_val;
		bool clear_depth;
		bool clear_stencil;
	};

	class GraphicsContext
	{
	public:
		GraphicsContext(ID3D11Device* device);

		void set_pso(const PSO_Handle pso_hnd);
		void set_vs_constant_buff(Constant_Buffer_Handle* cb_hnd, uint32_t start_slot, uint32_t num_slots);
		void set_ps_constant_buff(Constant_Buffer_Handle* cb_hnd, uint32_t start_slot, uint32_t num_slots);
		void set_ps_resource(Render_SRV_Handle* srv_hnd, uint32_t start_slot, uint32_t num_slots);
		void set_vertex_buff(Buffer_Handle_t buffer_hnd);
		void set_index_buff(Buffer_Handle_t buffer_hnd);
		void set_viewports(const Render_Rect* viewport, int32_t num_viewports);
		void set_render_targets(const Render_RTV_Handle* rt_hnds, const Render_DSV_Handle ds_hnds, uint32_t num_views);

		void set_pixel_shader(const Pixel_Shader* shader);
		void set_vertex_shader(const Vertex_Shader* shader);

		void update_subresource(const Constant_Buffer_Handle cb_hnd, void* data);
		
		//template <typename T>
		//void update_subresource(constant_buffer_handle<T> hnd_constant_buffer, const T& new_buffer_content)
		//{
		//	Constant_Buffer* constant_buffer = rhi.get_cb_pointer(hnd_constant_buffer);
		//	rhi.m_gfx_context->m_context->UpdateSubresource(constant_buffer->buffer, 0, nullptr, &new_buffer_content, 0, 0);
		//}

		void clear_dsv(const Render_DSV_Handle dsv_hnd, bool clear_depth = true, float depth_clear_val = 0.0f, bool clear_stencil = true, uint8_t stencil_clear_val = 0);
		void clear_rtv(const Render_RTV_Handle rtv_hnd, DirectX::XMFLOAT4& clear_color);

		//TODO: fix
		void get_ds_state(ID3D11DepthStencilState** ds_state);
		void set_ds_state(ID3D11DepthStencilState* ds_state);
		void update_subresource(Shader_Resource* resource);
		void set_ps_resource(Shader_Resource* resource);
		void set_ps_constant_buff(Shader_Resource* resource);

		void draw(Submesh_Handle_t submesh_hnd);
		void draw_cube();
		void draw_full_quad();
		void draw_indexed(Submesh_Handle_t submesh_hnd);
		ID3D11CommandList* finish_command_list();

		ID3D11Device* m_device;
		ID3D11DeviceContext* m_context;
		ID3D11CommandList* m_command_list;
		ID3DUserDefinedAnnotation* m_annot;
	};


	struct Render_Command_List
	{
		//TODO: fix; shouldn't pass anything that specifies backend API; this is a temporary solution
		Render_Command_List(DX11_Render_Device& render_device);

		//void draw(PSO_Handle _pso_hnd, std::vector<Shader_Resource>& shader_resources, const Buffer_Handle_t* _hnd_vert_buffers, uint32_t num_vert_buffers, Submesh_Handle_t hnd_submesh);
		void draw(PSO_Handle _pso_hnd, Shader_Render_Targets&& shader_render_targets, Shader_Arguments&& shader_arguments, const Buffer_Handle_t* _hnd_vert_buffers, uint32_t num_vert_buffers, D3D_PRIMITIVE_TOPOLOGY primitive_topology, Submesh_Handle_t hnd_submesh);
		//void draw_indexed(PSO_Handle _pso_hnd, std::vector<Shader_Resource>& shader_resources, Buffer_Handle_t _hnd_index_buffer, const Buffer_Handle_t* _hnd_vert_buffers, uint32_t num_vert_buffers, Submesh_Handle_t hnd_submesh);
		void draw_indexed(PSO_Handle _pso_hnd, Shader_Render_Targets&& shader_render_targets, Shader_Arguments&& shader_arguments, Buffer_Handle_t _hnd_index_buffer, const Buffer_Handle_t* _hnd_vert_buffers, uint32_t num_vert_buffers, D3D_PRIMITIVE_TOPOLOGY primitive_topology, Submesh_Handle_t hnd_submesh);
		void update_buffer(Arena& arena, Constant_Buffer_Handle hnd_cb, void* new_data, size_t size_in_bytes);
		void copy_texture(Render_RTV_Handle dst, Render_RTV_Handle src);
		void begin_event(const wchar_t* event_name);
		void end_event();
		void set_viewport(Render_Rect viewport);
		void clear_rtv(Render_RTV_Handle rt_hnd, float* clear_color);
		void clear_dsv(const Render_DSV_Handle dsv_hnd, bool clear_depth = true, float depth_clear_val = 0.0f, bool clear_stencil = true, uint8_t stencil_clear_val = 0);

		void clear();

		ID3D11CommandList* finish_command_list();

		std::vector<Render_Command*> cmd_list;

		//TODO: this shouldn't be here; probably should pass Render_Command_List to Command_Queue
		// and there gets compiled to API specific commands
		GraphicsContext gfx_context;

	};



}

#endif // !GRAPHICS_CONTEXT_H_
