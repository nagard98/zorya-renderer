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

namespace wrl = Microsoft::WRL;

#define MULTISAMPLE_COUNT 1

#define RETURN_IF_FAILED(hResult) { if(FAILED(hResult)) return hResult; }

typedef std::uint8_t RHI_RESULT;
#define RHI_OK std::uint8_t(0)
#define RHI_ERR std::uint8_t(1)

template <typename T, typename I>
struct StateVariantPtr {
	RHIState variant = -1;
	T* pStateObject;
};

class RenderHardwareInterface 
{

public:
	RenderHardwareInterface();
	~RenderHardwareInterface();

	HRESULT Init(HWND windowHandle, RHIState initialState = RHI_DEFAULT_STATE());

	void SetState(RHIState newState);
	RHI_RESULT LoadTexture(const wchar_t *path, ShaderTexture2D &shaderTexture, bool convertToLinear = true, size_t maxSize = 0);

	HRESULT ResizeWindow(std::uint32_t width, std::uint32_t height);

	void ReleaseAllResources();

	//wrl::ComPtr<ID3D11Device> device;
	DX11RenderDevice device;

	ID3D11DeviceContext* context;
	IDXGISwapChain* swapChain;
	
	ID3D11RenderTargetView* backBufferRTV;

	ID3D11Texture2D* backBufferDepthTex;
	ID3D11DepthStencilView* backBufferDepthDSV;
	ID3D11ShaderResourceView* backBufferDepthSRV;

	D3D_FEATURE_LEVEL featureLevel; //feature level found for device
	D3D11_VIEWPORT viewport;

private:
	RHIState state;

	std::array<StateVariantPtr<ID3D11RasterizerState, std::uint8_t>, MAX_STATE_VARIANTS> rastVariants;
	std::array<StateVariantPtr<ID3D11DepthStencilState, std::uint32_t>, MAX_STATE_VARIANTS> depthStenVariants;
	std::array<StateVariantPtr<ID3D11BlendState, std::uint32_t>, MAX_STATE_VARIANTS> blendVariants;
	std::array<StateVariantPtr<ID3D11SamplerState, std::uint16_t>, MAX_STATE_VARIANTS> samplerVariants;

	D3D11_RASTERIZER_DESC tmpRastDesc;
	D3D11_DEPTH_STENCIL_DESC tmpDepthStenDesc;
	
};

extern RenderHardwareInterface rhi;

#endif