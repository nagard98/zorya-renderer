#include <d3d11_1.h>
#include <wrl/client.h>

namespace wrl = Microsoft::WRL;

#define RETURN_IF_FAILED(hResult) { if(FAILED(hResult)) return hResult; }

class RenderHardwareInterface 
{

public:
	RenderHardwareInterface();
	~RenderHardwareInterface();

	HRESULT Init(HWND windowHandle);

	wrl::ComPtr<ID3D11Device> device;
	wrl::ComPtr<ID3D11DeviceContext> context;
	wrl::ComPtr<IDXGISwapChain> swapChain;
	wrl::ComPtr<ID3D11RenderTargetView> renderTargetView;

	D3D_FEATURE_LEVEL featureLevel; //feature level found for device
	D3D11_VIEWPORT viewport;
	
};

extern RenderHardwareInterface rhi;