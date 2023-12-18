#include <d3d11_1.h>
#include "imgui_impl_dx11.h"

#include <iostream>
#include <cstdint>

#include "RHIState.h"
#include "RenderHardwareInterface.h"
#include "Material.h"
#include "Editor/Logger.h"

#include "WICTextureLoader.h"

namespace dx = DirectX;

RenderHardwareInterface rhi;

RenderHardwareInterface::RenderHardwareInterface()
{
    
}

RenderHardwareInterface::~RenderHardwareInterface()
{
}

HRESULT RenderHardwareInterface::Init(HWND windowHandle, RHIState initialState)
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
    scd.SampleDesc.Count = MULTISAMPLE_COUNT;
    scd.SampleDesc.Quality = 0;
    scd.Windowed = TRUE;
    scd.OutputWindow = windowHandle;
    scd.BufferCount = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;

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

    RETURN_IF_FAILED(hRes);


    wrl::ComPtr<IDXGIFactory1> dxgiFactory1;
    {
        wrl::ComPtr<IDXGIDevice> dxgiDevice;
        hRes = (device)->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(dxgiDevice.GetAddressOf()));

        if (SUCCEEDED(hRes)) {
            wrl::ComPtr<IDXGIAdapter> adapter;
            hRes = dxgiDevice->GetAdapter(adapter.GetAddressOf());

            if (SUCCEEDED(hRes)) {
                adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(dxgiFactory1.GetAddressOf()));
            }
        }
    }

    RETURN_IF_FAILED(hRes);

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

    hRes = device->CreateRenderTargetView(backBuffer.Get(), nullptr, renderTargetView.GetAddressOf());
    RETURN_IF_FAILED(hRes);


    ID3D11Texture2D* rtTexture = nullptr;
    D3D11_TEXTURE2D_DESC rtTexDesc;
    ZeroMemory(&rtTexDesc, sizeof(rtTexDesc));
    rtTexDesc.Format = scd.BufferDesc.Format;
    rtTexDesc.Height = scd.BufferDesc.Height;
    rtTexDesc.Width = scd.BufferDesc.Width;
    rtTexDesc.MipLevels = 1;
    rtTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    rtTexDesc.Usage = D3D11_USAGE_DEFAULT;
    rtTexDesc.ArraySize = 1;
    rtTexDesc.SampleDesc.Count = 1;
    rtTexDesc.SampleDesc.Quality = 0;

    hRes = rhi.device->CreateTexture2D(&rtTexDesc, nullptr, &rtTexture);
    RETURN_IF_FAILED(hRes);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.Texture2D.MipLevels = -1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

    ID3D11ShaderResourceView* srvv = nullptr;
    hRes = device->CreateShaderResourceView(rtTexture, &srvDesc, rhi.renderTargetShaderResourceView.GetAddressOf());
    RETURN_IF_FAILED(hRes);

    (context)->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), nullptr);

    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = (FLOAT)width;
    viewport.Height = (FLOAT)height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    context->RSSetViewports(1, &viewport);

    SetState(initialState);

    //TODO:move sampler creation to somewhere else----------------
    D3D11_SAMPLER_DESC samplerDesc;
    ZeroMemory(&samplerDesc, sizeof(samplerDesc));
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    //samplerDesc.BorderColor[0] = 1.0f;
    //samplerDesc.BorderColor[1] = 1.0f;
    //samplerDesc.BorderColor[2] = 1.0f;
    //samplerDesc.BorderColor[3] = 1.0f;
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = FLT_MAX;
    samplerDesc.MaxAnisotropy = 16;

    hRes = device->CreateSamplerState(&samplerDesc, &samplerVariants[0].pStateObject);
    RETURN_IF_FAILED(hRes);

    context->PSSetSamplers(0, 1, &samplerVariants[0].pStateObject);
    //-----------------------------------------------------------------


    //TODO: move depth/stencil view creation somewhere else------------------
    D3D11_TEXTURE2D_DESC depthStenTexDesc;
    depthStenTexDesc.MipLevels = 1;
    depthStenTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    depthStenTexDesc.MiscFlags = 0;
    depthStenTexDesc.CPUAccessFlags = 0;
    depthStenTexDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStenTexDesc.Width = rhi.viewport.Width;
    depthStenTexDesc.Height = rhi.viewport.Height;
    depthStenTexDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    depthStenTexDesc.ArraySize = 1;
    depthStenTexDesc.SampleDesc.Count = MULTISAMPLE_COUNT;
    depthStenTexDesc.SampleDesc.Quality = 0;

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.Flags = 0;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    D3D11_SHADER_RESOURCE_VIEW_DESC dsRsvDesc;
    dsRsvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    dsRsvDesc.Texture2D.MipLevels = 1;
    dsRsvDesc.Texture2D.MostDetailedMip = 0;
    dsRsvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

    hRes = device->CreateTexture2D(&depthStenTexDesc, NULL, depthStencilBuff.GetAddressOf());
    RETURN_IF_FAILED(hRes);

    hRes = device->CreateDepthStencilView(depthStencilBuff.Get(), &dsvDesc, depthStencilView.GetAddressOf());
    RETURN_IF_FAILED(hRes);

    hRes = device->CreateShaderResourceView(depthStencilBuff.Get(), &dsRsvDesc, depthStencilShaderResourceView.GetAddressOf());
    RETURN_IF_FAILED(hRes);

    context->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencilView.Get());
    //--------------------------------------------------

    //TODO: move blend desc somewhere else-------------
    D3D11_RENDER_TARGET_BLEND_DESC targetBlendDesc1;
    targetBlendDesc1.BlendEnable = true;
    targetBlendDesc1.SrcBlend = D3D11_BLEND_SRC_ALPHA;
    targetBlendDesc1.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    targetBlendDesc1.BlendOp = D3D11_BLEND_OP_ADD;
    targetBlendDesc1.SrcBlendAlpha = D3D11_BLEND_ONE;
    targetBlendDesc1.DestBlendAlpha = D3D11_BLEND_ZERO;
    targetBlendDesc1.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    targetBlendDesc1.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;


    D3D11_BLEND_DESC blendDesc;
    blendDesc.AlphaToCoverageEnable = false;
    blendDesc.IndependentBlendEnable = false;
    blendDesc.RenderTarget[0] = targetBlendDesc1;

    hRes = device->CreateBlendState(&blendDesc, &blendVariants[0].pStateObject);
    RETURN_IF_FAILED(hRes);
    //rhi.context->OMSetBlendState(blendVariants[0].pStateObject, 0, 0);
    //---------------------------

    //TODO: move gui initialization elsewhere
    ImGui_ImplDX11_Init(device.Get(), context.Get());

    return S_OK;
}

void RenderHardwareInterface::SetState(RHIState newState)
{
    RHIState stateChanges = newState ^ state;

    // if state hasn't changed from last update, return
    if (stateChanges == 0) {
        return;
    }

    // Searches raster state object in cache
    if ((stateChanges & RASTER_STATE_MASK) != 0) {
        RHIState newRastState = newState & RASTER_STATE_MASK;
        
        for (int i = 0; i < rastVariants.size(); i++)
        {
            if (rastVariants[i].variant == -1){
                Logger::AddLog(Logger::Channel::TRACE, "Building new raster state\n");
                ZeroMemory(&tmpRastDesc, sizeof(tmpRastDesc));
                tmpRastDesc.FillMode = (newRastState & RASTER_FILL_MODE_MASK) == RASTER_FILL_MODE_SOLID ? D3D11_FILL_SOLID : D3D11_FILL_WIREFRAME;
                tmpRastDesc.AntialiasedLineEnable = (newRastState & RASTER_ANTIALIAS_MASK) == RASTER_ANTIALIAS_MASK;
                tmpRastDesc.CullMode = (newRastState & RASTER_CULL_MODE_MASK) == RASTER_CULL_FRONT ? D3D11_CULL_FRONT : D3D11_CULL_BACK;
                tmpRastDesc.MultisampleEnable = (newRastState & RASTER_MULTISAMPLE_MASK) == RASTER_MULTISAMPLE_MASK;
                tmpRastDesc.ScissorEnable = (newRastState & RASTER_SCISSOR_MASK) == RASTER_SCISSOR_MASK;

                rastVariants[i].variant = newRastState & RASTER_STATE_MASK;
                device->CreateRasterizerState(&tmpRastDesc, &(rastVariants[i].pStateObject));
                context->RSSetState(rastVariants[i].pStateObject);
                break;
            }
            else if (rastVariants[i].variant == newRastState) {
                context->RSSetState(rastVariants[i].pStateObject);
                break;
            }
        }
    }

    if ((stateChanges & DEPTH_STENCIL_STATE_MASK) != 0) {
        RHIState newDSState = newState & DEPTH_STENCIL_STATE_MASK;

        for (int i = 0; i < depthStenVariants.size(); i++)
        {
            if (depthStenVariants[i].variant == -1) {
                Logger::AddLog(Logger::Channel::TRACE, "Building new depth stancil state\n");
                ZeroMemory(&tmpDepthStenDesc, sizeof(tmpDepthStenDesc));
                tmpDepthStenDesc.DepthEnable = (newDSState & DEPTH_ENABLE_MASK) == DEPTH_ENABLE_MASK;
                tmpDepthStenDesc.DepthWriteMask = (newDSState & DEPTH_WRITE_ENABLE_MASK) == DEPTH_WRITE_ENABLE_MASK ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO ;
                tmpDepthStenDesc.DepthFunc = (newDSState & DEPTH_COMP_MASK) == DEPTH_COMP_LESS ? D3D11_COMPARISON_LESS : D3D11_COMPARISON_LESS_EQUAL;

                depthStenVariants[i].variant = newDSState;
                device->CreateDepthStencilState(&tmpDepthStenDesc, &(depthStenVariants[i].pStateObject));
                context->OMSetDepthStencilState(depthStenVariants[i].pStateObject, 0.0f);
                break;
            }
            else if (depthStenVariants[i].variant == newDSState) {
                context->OMSetDepthStencilState(depthStenVariants[i].pStateObject, 0.0f);
                break;
            }
        }
    }

    state = newState;

}

RHI_RESULT RenderHardwareInterface::LoadTexture(const wchar_t *path, ShaderTexture2D& shaderTexture, bool convertToLinear, size_t maxSize)
{
    if (shaderTexture.resourceView != nullptr) {
        ID3D11ShaderResourceView* nullViews[3] = { nullptr };
        context->PSSetShaderResources(0,3, nullViews);
        
        ID3D11Resource* textureRes = nullptr;
        shaderTexture.resourceView->GetResource(&textureRes);
        
        shaderTexture.resourceView->Release();
        shaderTexture.resourceView = nullptr;
        
        textureRes->Release();
        textureRes = nullptr;
    }

    HRESULT hRes = dx::CreateWICTextureFromFileEx(device.Get(), context.Get(),
        path,
        maxSize,
        D3D11_USAGE_DEFAULT,
        D3D11_BIND_SHADER_RESOURCE,
        0, 0, convertToLinear ? dx::WIC_LOADER_SRGB_DEFAULT : dx::WIC_LOADER_IGNORE_SRGB, NULL, &shaderTexture.resourceView);

    return hRes==S_OK ? RHI_OK : RHI_ERR;
}
