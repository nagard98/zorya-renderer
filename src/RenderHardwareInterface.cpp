#include <d3d11_1.h>
#include "imgui_impl_dx11.h"

#include "RenderHardwareInterface.h"

RenderHardwareInterface rhi;

RenderHardwareInterface::RenderHardwareInterface()
{
    
}

RenderHardwareInterface::~RenderHardwareInterface()
{
}

HRESULT RenderHardwareInterface::Init(HWND windowHandle)
{
    HRESULT hRes = S_OK;

    RECT rect;
    GetClientRect(windowHandle, &rect);
    UINT width = rect.right - rect.left;
    UINT height = rect.bottom - rect.top;

    UINT deviceFlags = 0;
#if _DEBUG
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE d3d_driver_supp[] = {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE
    };
    UINT driverTypesNum = ARRAYSIZE(d3d_driver_supp);

    D3D_FEATURE_LEVEL d3d_FL_supp[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
        /*D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0*/
    };
    UINT featureLevNum = ARRAYSIZE(d3d_FL_supp);

    DXGI_SWAP_CHAIN_DESC scd = {};
    SecureZeroMemory(&scd, sizeof(scd));

    scd.BufferDesc.Width = width;
    scd.BufferDesc.Height = height;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.SampleDesc.Count = 1;
    scd.SampleDesc.Quality = 0;
    scd.Windowed = TRUE;
    scd.OutputWindow = windowHandle;
    scd.BufferCount = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

    for (UINT driverIndex = 0; driverIndex < driverTypesNum; driverIndex++) {
        hRes = D3D11CreateDeviceAndSwapChain(
            NULL,
            d3d_driver_supp[driverIndex],
            NULL,
            deviceFlags,
            d3d_FL_supp,
            featureLevNum,
            D3D11_SDK_VERSION,
            &scd,
            swapChain.GetAddressOf(),
            &(device),
            &featureLevel,
            context.GetAddressOf()
        );

        if (SUCCEEDED(hRes)) break;
    }

    if (FAILED(hRes)) return hRes;



    wrl::ComPtr<IDXGIFactory1> dxgiFactory1;
    {
        wrl::ComPtr<IDXGIDevice> dxgiDevice;
        hRes = (device)->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(dxgiDevice.GetAddressOf()));

        if (SUCCEEDED(hRes)) {
            wrl::ComPtr<IDXGIAdapter> adapter;
            hRes = dxgiDevice->GetAdapter(adapter.GetAddressOf());

            if (SUCCEEDED(hRes)) {
                adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(dxgiFactory1.GetAddressOf()));
                //adapter->Release();
            }
            //dxgiDevice->Release();
        }
    }

    if (FAILED(hRes)) return hRes;

    //IDXGIFactory2* dxgiFactory2 = nullptr;
    //hRes = dxgiFactory1->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));

    //if (dxgiFactory2) {
    //    hRes = g_d3dDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&g_d3dDevice1));
    //    if (SUCCEEDED(hRes)) {
    //        (void)g_d3dDeviceContexQueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&g_d3dDeviceContext1));
    //    }

    //    DXGI_SWAP_CHAIN_DESC1 scd1 = {};
    //    SecureZeroMemory(&scd1, sizeof(DXGI_SWAP_CHAIN_DESC1));
    //    scd1.Stereo = FALSE;
    //    scd1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    //    scd1.SampleDesc.Count = 1;
    //    scd1.SampleDesc.Quality = 0;
    //    scd1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    //    //Include front buffer only if fullscreen
    //    scd1.BufferCount = 1;

    //    hRes = dxgiFactory2->CreateSwapChainForHwnd(g_d3dDevice, g_windowHandle, &scd1, nullptr, nullptr, &g_dxgiSwapChain1);
    //    if (SUCCEEDED(hRes)) hRes = g_dxgiSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&g_dxgiSwapChain));

    //    dxgiFactory2->Release();

    //}
    //else {
    //    DXGI_SWAP_CHAIN_DESC scd = {};
    //    SecureZeroMemory(&scd, sizeof(scd));

    //    scd.BufferDesc.Width = width;
    //    scd.BufferDesc.Height = height;
    //    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    //    scd.BufferDesc.RefreshRate.Numerator = 60;
    //    scd.BufferDesc.RefreshRate.Denominator = 1;
    //    scd.SampleDesc.Count = 1;
    //    scd.SampleDesc.Quality = 0;
    //    scd.Windowed = TRUE;
    //    scd.OutputWindow = g_windowHandle;
    //    scd.BufferCount = 1;
    //    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

    //    hRes = dxgiFactory1->CreateSwapChain(g_d3dDevice, &scd, &g_dxgiSwapChain);

    //}

    hRes = dxgiFactory1->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER);
    RETURN_IF_FAILED(hRes);

    wrl::ComPtr<ID3D11Texture2D> backBuffer;
    hRes = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));
    RETURN_IF_FAILED(hRes);

    hRes = (device)->CreateRenderTargetView(backBuffer.Get(), nullptr, renderTargetView.GetAddressOf());
    RETURN_IF_FAILED(hRes);

    (context)->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), nullptr);

    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = (FLOAT)width;
    viewport.Height = (FLOAT)height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    context->RSSetViewports(1, &viewport);

    //TODO: move gui initialization elsewhere
    ImGui_ImplDX11_Init(device.Get(), context.Get());

    return S_OK;
}
