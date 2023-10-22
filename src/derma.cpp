#define _CRTDBG_MAP_ALLOC

#include <stdlib.h>
#include <crtdbg.h>
#include <Windows.h>

#include <d3d11_1.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <DirectXColors.h>
#include <WICTextureLoader.h>

#include <wrl/client.h>

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include "../include/Camera.h";

#include <iostream>
#include <string>


#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")
#pragma comment (lib, "winmm.lib")


namespace dx = DirectX;
namespace wrl = Microsoft::WRL;

const LONG g_windowWidth = 1280;
const LONG g_windowHeight = 720;
LPCSTR g_windowClassName = "DirectXWindowClass";
LPCSTR g_windowName = "DirectX Window";
HWND g_windowHandle = 0;

const BOOL g_enableVSync = TRUE;

D3D_DRIVER_TYPE g_d3dDrivType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL g_d3dFeatureLevel = D3D_FEATURE_LEVEL_9_1;


wrl::ComPtr<ID3D11Device> g_d3dDevice;

wrl::ComPtr<ID3D11DeviceContext> g_d3dDeviceContext;

wrl::ComPtr<IDXGISwapChain> g_dxgiSwapChain;

wrl::ComPtr<ID3D11RenderTargetView> g_d3dRenderTargetView;
wrl::ComPtr<ID3D11DepthStencilView> g_d3dDepthStencilView;
wrl::ComPtr<ID3D11Texture2D> g_d3dDepthStencilBuffer;

wrl::ComPtr<ID3D11DepthStencilState> g_d3dDepthStencilState;
wrl::ComPtr<ID3D11RasterizerState> g_d3dRasterizerState;
wrl::ComPtr<ID3D11RasterizerState> g_d3dRasterizerStateCullBack;
wrl::ComPtr<ID3D11RasterizerState> g_d3dRasterizerStateCullFront;

wrl::ComPtr<ID3D11BlendState> g_d3dBlendState;

wrl::ComPtr<ID3D11InputLayout> g_d3dInputLayout;
wrl::ComPtr<ID3D11Buffer> g_d3dVertexBuffer;
wrl::ComPtr<ID3D11Buffer> g_d3dIndexBuffer;

wrl::ComPtr<ID3D11Buffer> g_cbPerObj;
wrl::ComPtr<ID3D11Buffer> g_cbPerCam;
wrl::ComPtr<ID3D11Buffer> g_cbPerProj;
wrl::ComPtr<ID3D11Buffer> g_cbLight;

wrl::ComPtr<ID3D11PixelShader> g_d3dPixelShader;
wrl::ComPtr<ID3D11VertexShader> g_d3dVertexShader;


D3D11_VIEWPORT g_Viewport = { 0 };
Camera g_cam;

template<class ShaderClass>
std::string GetLatestProfile();

template<>
std::string GetLatestProfile<ID3D11VertexShader>(){
    return "vs_5_0";
}

template<>
std::string GetLatestProfile<ID3D11PixelShader>() {
    return "ps_5_0";
}


template<class ShaderClass>
HRESULT CreateShader(ID3DBlob* pShaderBlob, ShaderClass** pShader, ID3D11Device* g_d3dDevice);

template<>
HRESULT CreateShader<ID3D11VertexShader>(ID3DBlob* pShaderBlob, ID3D11VertexShader** pShader, ID3D11Device *g_d3dDevice) 
{
    return g_d3dDevice->CreateVertexShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, pShader);
};

template<>
HRESULT CreateShader<ID3D11PixelShader>(ID3DBlob* pShaderBlob, ID3D11PixelShader** pShader, ID3D11Device* g_d3dDevice)
{
    return g_d3dDevice->CreatePixelShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, pShader);
};

template<class ShaderClass>
HRESULT LoadShader(const std::wstring& shaderFilename, const std::string& entrypoint, ID3DBlob** shaderBlob, ShaderClass** shader, ID3D11Device *g_d3dDevice) {

    wrl::ComPtr<ID3DBlob> errBlob = nullptr;

    std::string profile = GetLatestProfile<ShaderClass>();

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG;

    HRESULT hr = D3DCompileFromFile(shaderFilename.c_str(), NULL, NULL, entrypoint.c_str(), profile.c_str(), flags, 0, shaderBlob, errBlob.GetAddressOf());

    if (FAILED(hr)) {
        std::string errorMessage = (char*)errBlob->GetBufferPointer();
        OutputDebugStringA(errorMessage.c_str());

        return hr;
    }
     
    hr = CreateShader<ShaderClass>(*shaderBlob, shader, g_d3dDevice);
    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}

enum ConstanBuffer {
    CB_Application,
    CB_Frame,
    CB_Object,
    CB_Light,
    NumConstantBuffers
};

ID3D11Buffer* g_d3dConstanBuffers[NumConstantBuffers];

dx::XMMATRIX g_WorldMatrix;
dx::XMMATRIX g_ViewMatrix;
dx::XMMATRIX g_projMatrix;

D3D11_INPUT_ELEMENT_DESC vertexLayouts[] = {
    {"position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"texcoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
};

struct Vertex
{
    Vertex() {}
    Vertex(float x, float y, float z,
        float u, float v, float nx, float ny, float nz)
        : position(x, y, z), texCoord(u, v), normal(nx, ny, nz) {}

    dx::XMFLOAT3 position;
    dx::XMFLOAT2 texCoord;
    dx::XMFLOAT3 normal;
};

Vertex g_Vertices[] =
{
    // Front Face
    Vertex(-1.0f, -1.0f, -1.0f, 0.0f, 1.0f,-1.0f, -1.0f, -1.0f),
    Vertex(-1.0f,  1.0f, -1.0f, 0.0f, 0.0f,-1.0f,  1.0f, -1.0f),
    Vertex(1.0f,  1.0f, -1.0f, 1.0f, 0.0f, 1.0f,  1.0f, -1.0f),
    Vertex(1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f),

    // Back Face
    Vertex(-1.0f, -1.0f, 1.0f, 1.0f, 1.0f,-1.0f, -1.0f, 1.0f),
    Vertex(1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, -1.0f, 1.0f),
    Vertex(1.0f,  1.0f, 1.0f, 0.0f, 0.0f, 1.0f,  1.0f, 1.0f),
    Vertex(-1.0f,  1.0f, 1.0f, 1.0f, 0.0f,-1.0f,  1.0f, 1.0f),

    // Top Face
    Vertex(-1.0f, 1.0f, -1.0f, 0.0f, 1.0f,-1.0f, 1.0f, -1.0f),
    Vertex(-1.0f, 1.0f,  1.0f, 0.0f, 0.0f,-1.0f, 1.0f,  1.0f),
    Vertex(1.0f, 1.0f,  1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f),
    Vertex(1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f),

    // Bottom Face
    Vertex(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f,-1.0f, -1.0f, -1.0f),
    Vertex(1.0f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, -1.0f),
    Vertex(1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 1.0f, -1.0f,  1.0f),
    Vertex(-1.0f, -1.0f,  1.0f, 1.0f, 0.0f,-1.0f, -1.0f,  1.0f),

    // Left Face
    Vertex(-1.0f, -1.0f,  1.0f, 0.0f, 1.0f,-1.0f, -1.0f,  1.0f),
    Vertex(-1.0f,  1.0f,  1.0f, 0.0f, 0.0f,-1.0f,  1.0f,  1.0f),
    Vertex(-1.0f,  1.0f, -1.0f, 1.0f, 0.0f,-1.0f,  1.0f, -1.0f),
    Vertex(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f,-1.0f, -1.0f, -1.0f),

    // Right Face
    Vertex(1.0f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, -1.0f),
    Vertex(1.0f,  1.0f, -1.0f, 0.0f, 0.0f, 1.0f,  1.0f, -1.0f),
    Vertex(1.0f,  1.0f,  1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  1.0f),
    Vertex(1.0f, -1.0f,  1.0f, 1.0f, 1.0f, 1.0f, -1.0f,  1.0f),
};

WORD g_Indices[36] =
{
    // Front Face
    0,  1,  2,
    0,  2,  3,

    // Back Face
    4,  5,  6,
    4,  6,  7,

    // Top Face
    8,  9, 10,
    8, 10, 11,

    // Bottom Face
    12, 13, 14,
    12, 14, 15,

    // Left Face
    16, 17, 18,
    16, 18, 19,

    // Right Face
    20, 21, 22,
    20, 22, 23
};

struct ObjCB {
    
    dx::XMMATRIX worldMatrix;

};

ObjCB objCB = { dx::XMMatrixTranspose( dx::XMMatrixTranslation(-2.0f, -2.0f, 4.0f) ) };

struct ViewCB {

    dx::XMMATRIX viewMatrix;

};

ViewCB viewCB;

struct ProjCB {

    dx::XMMATRIX projMatrix;

};

struct LightCB {
    dx::XMVECTOR lightDir;
};

LightCB lightCB = { dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f) };


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool LoadContent();
bool UnloadContent();

void Update(float deltaTime);
void Render();
void Cleanup();

int InitApplication(HINSTANCE hInstance, int cmdShow) {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;


    WNDCLASSEX wndClassEx = { 0 };
    wndClassEx.cbSize = sizeof(WNDCLASSEX);
    wndClassEx.style = CS_HREDRAW | CS_VREDRAW;
    wndClassEx.lpfnWndProc = &WndProc;
    wndClassEx.hInstance = hInstance;
    wndClassEx.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndClassEx.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClassEx.lpszClassName = g_windowClassName;
    wndClassEx.lpszMenuName = nullptr;

    if (!RegisterClassEx(&wndClassEx)) {
        return -1;
    }

    RECT windowRect = { 0,0,g_windowWidth,g_windowHeight };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    g_windowHandle = CreateWindow(
        g_windowClassName,
        g_windowName,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr, nullptr, hInstance, nullptr);

    if (!g_windowHandle) {
        return -1;
    }

    ImGui_ImplWin32_Init(g_windowHandle);

    ShowWindow(g_windowHandle, cmdShow);
    UpdateWindow(g_windowHandle);

    return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT paintStruct;
    HDC hDC;

    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_PAINT:
        hDC = BeginPaint(hwnd, &paintStruct);
        EndPaint(hwnd, &paintStruct);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return LRESULT();
}

int Run() {
    MSG msg = { 0 };

    static DWORD previousTime = timeGetTime();
    static const float targetFramerate = 60.0f;
    static const float maxTimeStep = 1.0f / targetFramerate;

    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            DWORD currentTime = timeGetTime();
            float deltaTime = (currentTime - previousTime) / 1000.0f;
            previousTime = currentTime;
            deltaTime = std::min<float>(deltaTime, maxTimeStep);

            Update(deltaTime);
            Render();
        }
    }



    return static_cast<int>(msg.wParam);

}

HRESULT InitDevice() {

    HRESULT hRes = S_OK;

    RECT rect;
    GetClientRect(g_windowHandle, &rect);
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
    scd.OutputWindow = g_windowHandle;
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
            g_dxgiSwapChain.GetAddressOf(),
            &(g_d3dDevice),
            &g_d3dFeatureLevel,
            g_d3dDeviceContext.GetAddressOf()
        );

        if (SUCCEEDED(hRes)) break;
    }

    if (FAILED(hRes)) return hRes;



    wrl::ComPtr<IDXGIFactory1> dxgiFactory1;
    {
        wrl::ComPtr<IDXGIDevice> dxgiDevice;
        hRes = (g_d3dDevice)->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(dxgiDevice.GetAddressOf()));

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

    hRes = dxgiFactory1->MakeWindowAssociation(g_windowHandle, DXGI_MWA_NO_ALT_ENTER);

    if (FAILED(hRes)) return hRes;

    wrl::ComPtr<ID3D11Texture2D> backBuffer ;
    hRes = g_dxgiSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));
    if (FAILED(hRes)) return hRes;

    hRes = (g_d3dDevice)->CreateRenderTargetView(backBuffer.Get(), nullptr, g_d3dRenderTargetView.GetAddressOf());
    if (FAILED(hRes)) return hRes;

    (g_d3dDeviceContext)->OMSetRenderTargets(1, g_d3dRenderTargetView.GetAddressOf(), nullptr);

    g_Viewport.TopLeftX = 0;
    g_Viewport.TopLeftY = 0;
    g_Viewport.Width = (FLOAT)width;
    g_Viewport.Height = (FLOAT)height;
    g_Viewport.MinDepth = 0.0f;
    g_Viewport.MaxDepth = 1.0f;

    g_d3dDeviceContext->RSSetViewports(1, &g_Viewport);

    ImGui_ImplDX11_Init(g_d3dDevice.Get(), g_d3dDeviceContext.Get());

    return S_OK;
}


HRESULT InitData() {

    wrl::ComPtr<ID3D11InputLayout> vertLayout ;
    wrl::ComPtr<ID3D11InputLayout> pixLayout ;

    wrl::ComPtr<ID3DBlob> verShaderBlob ;
    wrl::ComPtr<ID3DBlob> pixShaderBlob;
    
    HRESULT hr;

    hr = LoadShader<ID3D11VertexShader>(L"shaders/BasicVertexShader.hlsl", "vs", verShaderBlob.GetAddressOf(), g_d3dVertexShader.GetAddressOf(), g_d3dDevice.Get());
    if (FAILED(hr))
    {
        return hr;
    }

    hr = LoadShader<ID3D11PixelShader>(L"./shaders/BasicPixelShader.hlsl", "ps", pixShaderBlob.GetAddressOf(), g_d3dPixelShader.GetAddressOf(), g_d3dDevice.Get());
    if (FAILED(hr))
    {
        return hr;
    }

    D3D11_BUFFER_DESC vertBuffDesc;
    vertBuffDesc.Usage = D3D11_USAGE_DEFAULT;
    vertBuffDesc.ByteWidth = sizeof(g_Vertices);
    vertBuffDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertBuffDesc.CPUAccessFlags = 0;
    vertBuffDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA vertSubresource;
    vertSubresource.pSysMem = g_Vertices;
    vertSubresource.SysMemPitch = 0;
    vertSubresource.SysMemSlicePitch = 0;

    hr = g_d3dDevice->CreateBuffer(&vertBuffDesc, &vertSubresource, g_d3dVertexBuffer.GetAddressOf());
    if (FAILED(hr)) return hr;

    UINT strides[] = { sizeof(Vertex) };
    UINT offsets[] = { 0 };
    g_d3dDeviceContext->IASetVertexBuffers(0, 1, g_d3dVertexBuffer.GetAddressOf(), strides, offsets);

    D3D11_BUFFER_DESC indxBuffDesc;
    indxBuffDesc.Usage = D3D11_USAGE_DEFAULT;
    indxBuffDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indxBuffDesc.CPUAccessFlags = 0;
    indxBuffDesc.ByteWidth = sizeof(WORD) * 36;
    indxBuffDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA indxSubresource;
    indxSubresource.pSysMem = g_Indices;
    indxSubresource.SysMemPitch = 0;
    indxSubresource.SysMemSlicePitch = 0;

    hr = g_d3dDevice->CreateBuffer(&indxBuffDesc, &indxSubresource, g_d3dIndexBuffer.GetAddressOf());
    if (FAILED(hr)) return hr;
    g_d3dDeviceContext->IASetIndexBuffer(g_d3dIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

    g_d3dDeviceContext->VSSetShader(g_d3dVertexShader.Get(), 0, 0);
    g_d3dDeviceContext->PSSetShader(g_d3dPixelShader.Get(), 0, 0);

    hr = g_d3dDevice->CreateInputLayout(vertexLayouts, 3, verShaderBlob->GetBufferPointer(), verShaderBlob->GetBufferSize(), vertLayout.GetAddressOf());

    g_d3dDeviceContext->IASetInputLayout(vertLayout.Get());
    g_d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D11_TEXTURE2D_DESC depthStenTexDesc;
    depthStenTexDesc.MipLevels = 1;
    depthStenTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStenTexDesc.MiscFlags = 0;
    depthStenTexDesc.CPUAccessFlags = 0;
    depthStenTexDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStenTexDesc.Width = g_Viewport.Width;
    depthStenTexDesc.Height = g_Viewport.Height;
    depthStenTexDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStenTexDesc.ArraySize = 1;
    depthStenTexDesc.SampleDesc.Count = 1;
    depthStenTexDesc.SampleDesc.Quality = 0;


    hr = g_d3dDevice->CreateTexture2D(&depthStenTexDesc, NULL, (g_d3dDepthStencilBuffer).GetAddressOf());
    if (FAILED(hr)) {
        return hr;
    }

    hr = g_d3dDevice->CreateDepthStencilView(g_d3dDepthStencilBuffer.Get(), NULL, g_d3dDepthStencilView.GetAddressOf());
    if (FAILED(hr)) {
        return hr;
    }

    g_d3dDeviceContext->OMSetRenderTargets(1, g_d3dRenderTargetView.GetAddressOf(), g_d3dDepthStencilView.Get());

    D3D11_SUBRESOURCE_DATA resourceCbPerObj;
    resourceCbPerObj.pSysMem = &objCB;
    resourceCbPerObj.SysMemPitch = 0;
    resourceCbPerObj.SysMemSlicePitch = 0;

    D3D11_BUFFER_DESC cbObjDesc;
    cbObjDesc.Usage = D3D11_USAGE_DEFAULT;
    cbObjDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbObjDesc.CPUAccessFlags = 0;
    cbObjDesc.MiscFlags = 0;
    cbObjDesc.ByteWidth = sizeof(ObjCB);


    hr = g_d3dDevice->CreateBuffer(&cbObjDesc, &resourceCbPerObj, g_cbPerObj.GetAddressOf());
    if (FAILED(hr)) {
        return hr;
    }

    g_d3dDeviceContext->VSSetConstantBuffers(0, 1, g_cbPerObj.GetAddressOf());


    dx::XMVECTOR camPos = dx::XMVectorSet(0.0f, 0.0f, -8.0f, 0.0f);
    dx::XMVECTOR camDir = dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    dx::XMVECTOR camUp = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    //g_ViewMatrix = dx::XMMatrixLookAtLH(camPos, camDir, camUp);

    //viewCB.viewMatrix = dx::XMMatrixTranspose(g_ViewMatrix);

    g_cam = Camera(camPos, camDir, camUp, 60.0f * (3.14f/180.0f), g_Viewport.Width / g_Viewport.Height, 1.0f, 100.0f);

    viewCB.viewMatrix = g_cam.getViewMatrix();//dx::XMMatrixTranspose(g_ViewMatrix);

    D3D11_SUBRESOURCE_DATA resourceCbPerCam;
    resourceCbPerCam.pSysMem = &viewCB;
    resourceCbPerCam.SysMemPitch = 0;
    resourceCbPerCam.SysMemSlicePitch = 0;

    D3D11_BUFFER_DESC cbCamDesc;
    cbCamDesc.Usage = D3D11_USAGE_DEFAULT;
    cbCamDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbCamDesc.CPUAccessFlags = 0;
    cbCamDesc.MiscFlags = 0;
    cbCamDesc.ByteWidth = sizeof(viewCB);


    hr = g_d3dDevice->CreateBuffer(&cbCamDesc, &resourceCbPerCam, g_cbPerCam.GetAddressOf());
    if (FAILED(hr)) {
        return hr;
    }

    g_d3dDeviceContext->VSSetConstantBuffers(1, 1, g_cbPerCam.GetAddressOf());


    //g_projMatrix = dx::XMMatrixPerspectiveFovLH(0.4f * 3.14f, g_Viewport.Width / g_Viewport.Height, 1.0f, 100.0f);

    ProjCB projCB;
    projCB.projMatrix = g_cam.getProjMatrix();//dx::XMMatrixTranspose(g_projMatrix);

    D3D11_SUBRESOURCE_DATA resourceCbPerProj;
    resourceCbPerProj.pSysMem = &projCB;
    resourceCbPerProj.SysMemPitch = 0;
    resourceCbPerProj.SysMemSlicePitch = 0;

    D3D11_BUFFER_DESC cbProjDesc;
    cbProjDesc.Usage = D3D11_USAGE_DEFAULT;
    cbProjDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbProjDesc.CPUAccessFlags = 0;
    cbProjDesc.MiscFlags = 0;
    cbProjDesc.ByteWidth = sizeof(projCB);


    hr = g_d3dDevice->CreateBuffer(&cbProjDesc, &resourceCbPerProj, g_cbPerProj.GetAddressOf());
    if (FAILED(hr)) {
        return hr;
    }

    g_d3dDeviceContext->VSSetConstantBuffers(2, 1, g_cbPerProj.GetAddressOf());

    lightCB.lightDir = dx::XMVector4Transform(lightCB.lightDir, g_cam.getViewMatrix());

    D3D11_SUBRESOURCE_DATA lightResource;
    lightResource.pSysMem = &lightCB;
    lightResource.SysMemPitch = 0;
    lightResource.SysMemSlicePitch = 0;

    D3D11_BUFFER_DESC lightBuffDesc;
    ZeroMemory(&lightBuffDesc, sizeof(lightBuffDesc));
    lightBuffDesc.Usage = D3D11_USAGE_DEFAULT;
    lightBuffDesc.ByteWidth = sizeof(lightCB);
    
    g_d3dDevice->CreateBuffer(&lightBuffDesc, &lightResource, g_cbLight.GetAddressOf());

    g_d3dDeviceContext->PSSetConstantBuffers(0, 1, g_cbLight.GetAddressOf());

    //D3D11_RASTERIZER_DESC rastDesc;
    //ZeroMemory(&rastDesc, sizeof(rastDesc));

    //rastDesc.CullMode = D3D11_CULL_NONE;
    //rastDesc.FillMode = D3D11_FILL_SOLID

    //g_d3dDevice->CreateRasterizerState(&rastDesc, &g_d3dRasterizerState);
    //g_d3dDeviceContexRSSetState(g_d3dRasterizerState);

    D3D11_RASTERIZER_DESC rastDesc;
    ZeroMemory(&rastDesc, sizeof(rastDesc));
    
    rastDesc.CullMode = D3D11_CULL_BACK;
    rastDesc.FillMode = D3D11_FILL_SOLID;

    g_d3dDevice->CreateRasterizerState(&rastDesc, g_d3dRasterizerStateCullBack.GetAddressOf());

    rastDesc.CullMode = D3D11_CULL_FRONT;
    g_d3dDevice->CreateRasterizerState(&rastDesc, g_d3dRasterizerStateCullFront.GetAddressOf());


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

    g_d3dDevice->CreateBlendState(&blendDesc, g_d3dBlendState.GetAddressOf());

    wrl::ComPtr<ID3D11ShaderResourceView> textureView;
    hr = dx::CreateWICTextureFromFile(g_d3dDevice.Get(), L"./shaders/glass.png", NULL, textureView.GetAddressOf());
    if (FAILED(hr)) {
        return hr;
    }

    D3D11_SAMPLER_DESC samplerDesc;
    ZeroMemory(&samplerDesc, sizeof(samplerDesc));
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = FLT_MAX;
    samplerDesc.MaxAnisotropy = 16;


    wrl::ComPtr<ID3D11SamplerState> samplerState ;
    hr = g_d3dDevice->CreateSamplerState(&samplerDesc, samplerState.GetAddressOf());
    if (FAILED(hr)) {
        return hr;
    }

    g_d3dDeviceContext->PSSetSamplers(0, 1, samplerState.GetAddressOf());
    g_d3dDeviceContext->PSSetShaderResources(0, 1, textureView.GetAddressOf());

    return hr;
}

float rot = 0.0f;

void Render() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();

    g_d3dDeviceContext->ClearRenderTargetView(g_d3dRenderTargetView.Get(), dx::Colors::Black);
    g_d3dDeviceContext->ClearDepthStencilView(g_d3dDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    g_cam.rotate(0.0f, -0.003f, 0.0f);
    viewCB.viewMatrix = g_cam.getViewMatrix();
    g_d3dDeviceContext->UpdateSubresource(g_cbPerCam.Get(), 0, NULL, &viewCB, 0, 0);
    float blendFactor[4] = { 0.5f, 0.5f, 0.5f, 0.5f };


    //Update light direction with view matrix
    lightCB.lightDir = dx::XMVector4Transform(dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), g_cam._viewMatrix);
    g_d3dDeviceContext->UpdateSubresource(g_cbLight.Get(), 0, NULL, &lightCB, 0, 0);

    //g_d3dDeviceContexOMSetBlendState(g_d3dBlendState, blendFactor, 0xffffffff);
    
    dx::XMMATRIX transMat = dx::XMMatrixTranslation(0.0f, 0.0f, 7.0f);
    objCB.worldMatrix = dx::XMMatrixTranspose(dx::XMMatrixRotationAxis(dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rot) * transMat);//dx::XMMatrixRotationAxis(dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), 0.05f);
    g_d3dDeviceContext->UpdateSubresource(g_cbPerObj.Get(), 0, NULL, &objCB, 0, 0);

    g_d3dDeviceContext->RSSetState(g_d3dRasterizerStateCullFront.Get());
    g_d3dDeviceContext->DrawIndexed(36, 0, 0);
    g_d3dDeviceContext->RSSetState(g_d3dRasterizerStateCullBack.Get());
    g_d3dDeviceContext->DrawIndexed(36, 0, 0);

    transMat = dx::XMMatrixTranslation(-2.0f, -0.5f, 4.0f);
    objCB.worldMatrix = dx::XMMatrixTranspose(dx::XMMatrixRotationAxis(dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rot) * transMat);//dx::XMMatrixRotationAxis(dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), 0.05f);
    g_d3dDeviceContext->UpdateSubresource(g_cbPerObj.Get(), 0, NULL, &objCB, 0, 0);

    g_d3dDeviceContext->RSSetState(g_d3dRasterizerStateCullFront.Get());
    g_d3dDeviceContext->DrawIndexed(36, 0, 0);
    g_d3dDeviceContext->RSSetState(g_d3dRasterizerStateCullBack.Get());
    g_d3dDeviceContext->DrawIndexed(36, 0, 0);

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    g_dxgiSwapChain->Present(1, 0);
}

void Update(float deltaTime) {
    rot += (0.5f * deltaTime);
    if (rot > 6.28f) {
        rot = 0.0f;
    }
    rot = 0.0f;
}

void Cleanup() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (g_d3dDeviceContext) {
        g_d3dDeviceContext->ClearState();
        g_d3dDeviceContext->Flush();
    }

//    if (g_cbPerObj) g_cbPerObj->Release();
//    if (g_cbPerCam) g_cbPerCam->Release();
//    if (g_d3dDevice) g_d3dDevice->Release();
//    if (g_d3dDeviceContext) g_d3dDevice->Release();
//    if (g_dxgiSwapChain) g_dxgiSwapChain->Release();
 //   if (g_d3dDevice1) g_d3dDevice1->Release();
 //   if (g_d3dDeviceContext1) g_d3dDeviceContext1->Release();
 //   if (g_dxgiSwapChain1) g_dxgiSwapChain1->Release();

}



int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(pCmdLine);


    if (!dx::XMVerifyCPUSupport()) {
        MessageBox(NULL, "DirectXMath not supported by CPU", "Error", MB_OK);
        return -1;
    }

    if (InitApplication(hInstance, nCmdShow) != 0) {
        MessageBox(NULL, "Failed to create application window", "Error", MB_OK);
        return -1;
    }

    if (FAILED(InitDevice())) {
        MessageBox(NULL, "Failed to initialize correctly d3d device", "Error", MB_OK);
        Cleanup();
        return -1;
    }

    if (FAILED(InitData())) {
        MessageBox(NULL, "Failed to initialize correctly scene data", "Error", MB_OK);
        Cleanup();
        return -1;
    }

    int returnCode = Run();

    Cleanup();
    

    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtDumpMemoryLeaks();


    return returnCode;
}