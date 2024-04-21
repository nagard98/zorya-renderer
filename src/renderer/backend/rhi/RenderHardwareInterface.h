#ifndef RENDER_HARDWARE_INTERFACE_H_
#define RENDER_HARDWARE_INTERFACE_H_

#include <d3d11_1.h>
#include <wrl/client.h>
#include <cstdint>
#include <array>

#include "renderer/backend/rhi/RHIState.h"
#include "renderer/backend/rhi/RenderDevice.h"

#include "renderer/frontend/Material.h"
#include "renderer/frontend/Shader.h"

#define MULTISAMPLE_COUNT 1

#define RETURN_IF_FAILED(h_result) { if(FAILED(h_result)) return h_result; }

#define RHI_OK uint8_t(0)
#define RHI_ERR uint8_t(1)

namespace zorya
{
	namespace wrl = Microsoft::WRL;

	typedef uint8_t RHI_RESULT;

	template <typename T, typename I>
	struct State_Variant_Ptr
	{
		RHI_State variant = -1;
		T* p_state_object;
	};

	class Render_Hardware_Interface
	{

	public:
		Render_Hardware_Interface();
		~Render_Hardware_Interface();

		HRESULT init(HWND window_handle, RHI_State initial_state = RHI_DEFAULT_STATE());

		void set_state(RHI_State new_state);
		RHI_RESULT load_texture(const wchar_t* path, Shader_Texture2D& shader_texture, bool convert_to_linear = true, size_t max_size = 0);

		HRESULT resize_window(uint32_t width, uint32_t height);

		ID3D11BlendState* get_blend_state(int index);

		void release_all_resources();

		//wrl::ComPtr<ID3D11Device> device;
		DX11_Render_Device m_device;

		ID3D11DeviceContext* m_context;
		IDXGISwapChain* m_swap_chain;

		ID3D11RenderTargetView* m_back_buffer_rtv;

		ID3D11Texture2D* m_back_buffer_depth_tex;
		ID3D11DepthStencilView* m_back_buffer_depth_dsv;
		ID3D11ShaderResourceView* m_back_buffer_depth_srv;

		D3D_FEATURE_LEVEL m_feature_level; //feature level found for device
		D3D11_VIEWPORT m_viewport;

	private:
		RHI_State state;

		std::array<State_Variant_Ptr<ID3D11RasterizerState, uint8_t>, MAX_STATE_VARIANTS> m_rast_state_variants;
		std::array<State_Variant_Ptr<ID3D11DepthStencilState, uint32_t>, MAX_STATE_VARIANTS> m_depth_stencil_state_variants;
		std::array<State_Variant_Ptr<ID3D11BlendState, uint32_t>, MAX_STATE_VARIANTS> m_blend_state_variants;
		std::array<State_Variant_Ptr<ID3D11SamplerState, uint16_t>, MAX_STATE_VARIANTS> m_sampler_state_variants;

		D3D11_RASTERIZER_DESC m_tmp_rast_desc;
		D3D11_DEPTH_STENCIL_DESC m_tmp_depth_sten_desc;

	};

	extern Render_Hardware_Interface rhi;

}
#endif