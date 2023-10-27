#define _CRTDBG_MAP_ALLOC

#include <stdlib.h>
#include <crtdbg.h>
#include <Windows.h>
#include <hidusage.h>

#include <d3d11_1.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <DirectXColors.h>
#include <WICTextureLoader.h>
#include <DDSTextureLoader.h>

#include <wrl/client.h>

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include "../include/Camera.h"
#include "../include/Lights.h"
#include "../include/Shaders.h"
#include "../include/Mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <vector>
#include <string>
#include <cstdint>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")
#pragma comment (lib, "winmm.lib")

#define RETURN_IF_FAILED(hResult) { if(FAILED(hResult)) return hr; }

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

Assimp::Importer importer;

wrl::ComPtr<ID3D11Device> g_d3dDevice;

wrl::ComPtr<ID3D11DeviceContext> g_d3dDeviceContext;

wrl::ComPtr<IDXGISwapChain> g_dxgiSwapChain;

wrl::ComPtr<ID3D11RenderTargetView> g_d3dRenderTargetView;
wrl::ComPtr<ID3D11DepthStencilView> g_d3dDepthStencilView;
wrl::ComPtr<ID3D11Texture2D> g_d3dDepthStencilBuffer;

wrl::ComPtr<ID3D11ShaderResourceView> textureView;
wrl::ComPtr<ID3D11ShaderResourceView> cubemapView;

wrl::ComPtr<ID3D11DepthStencilState> g_d3dDepthStencilStateSkybox;
wrl::ComPtr<ID3D11RasterizerState> g_d3dRasterizerState;
wrl::ComPtr<ID3D11RasterizerState> g_d3dRasterizerStateCullBack;
wrl::ComPtr<ID3D11RasterizerState> g_d3dRasterizerStateCullFront;
wrl::ComPtr<ID3D11RasterizerState> g_d3dRasterizerStateSkybox;

wrl::ComPtr<ID3D11BlendState> g_d3dBlendState;

wrl::ComPtr<ID3D11InputLayout> g_d3dInputLayout;
wrl::ComPtr<ID3D11Buffer> g_d3dVertexBuffer;
wrl::ComPtr<ID3D11Buffer> g_d3dIndexBuffer;

wrl::ComPtr<ID3D11Buffer> g_d3dVertexBufferSkybox;
wrl::ComPtr<ID3D11Buffer> g_d3dIndexBufferSkybox;

wrl::ComPtr<ID3D11Buffer> g_cbPerObj;
wrl::ComPtr<ID3D11Buffer> g_cbPerCam;
wrl::ComPtr<ID3D11Buffer> g_cbPerProj;
wrl::ComPtr<ID3D11Buffer> g_cbLight;

wrl::ComPtr<ID3D11PixelShader> g_d3dPixelShader;
wrl::ComPtr<ID3D11VertexShader> g_d3dVertexShader;

wrl::ComPtr<ID3D11PixelShader> g_d3dPixelShaderSkybox;
wrl::ComPtr<ID3D11VertexShader> g_d3dVertexShaderSkybox;

wrl::ComPtr<ID3D11SamplerState> samplerState;
wrl::ComPtr<ID3D11SamplerState> samplerStateCube;

D3D11_VIEWPORT g_Viewport = { 0 };
Camera g_cam;


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

Vertex g_Vertices2[] =
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

WORD g_Indices2[36] =
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

//Vertex *g_Vertices;
int numVertices = 0;

std::uint16_t *g_Indices;
int numFaces = 0;



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


DirectionalLight dLight = { dx::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f) };
PointLight pLight1 = { dx::XMVectorSet(0.0f, 2.0f, 4.0f, 1.0f), 1.0f, 0.22f, 0.20f };

LightCB lightCB;

struct Mouse {
    std::int64_t relX, relY;
};

Mouse mouse;

//-----------------------------------------------------

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

    RAWINPUTDEVICE rid[1];
    rid[0].hwndTarget = g_windowHandle;
    rid[0].dwFlags = RIDEV_INPUTSINK;
    rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;

    RegisterRawInputDevices(rid, 1, sizeof(rid[0]));

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
    case WM_INPUT:
    {
        UINT size = sizeof(RAWINPUT);
        static BYTE lpb[sizeof(RAWINPUT)];
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &size, sizeof(RAWINPUTHEADER));

        RAWINPUT* raw = (RAWINPUT*)lpb;

        if (raw->header.dwType == RIM_TYPEMOUSE) {
            mouse.relX = raw->data.mouse.lLastX;
            mouse.relY = raw->data.mouse.lLastY;
        }

        break;
    }
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

dx::XMMATRIX scaleMat;

HRESULT InitData() {

    HRESULT hr;
    RenderSystem renderSys;
    
    const aiScene* scene = importer.ReadFile("./shaders/stanford-bunny.obj", aiProcess_GenSmoothNormals | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

    if(scene == nullptr)
    {
        std::cout << importer.GetErrorString() << std::endl;
        return E_ABORT;
    }

    aiMesh *mesh = scene->mMeshes[0];
    aiMatrix4x4 sM = scene->mRootNode->mTransformation;
    scaleMat = dx::XMMatrixScaling(/*sM.a1, sM.b2, sM.c3*/20.0f, 20.0f, 20.0f);

    numVertices = mesh->mNumVertices;
    //g_Vertices = new Vertex[numVertices];
    renderSys.bufLengths.push_back(numVertices);
    for (int i = 0; i < numVertices; i++) 
    {
        aiVector3D *vert = &(mesh->mVertices[i]);
        aiVector3D *normal = &(mesh->mNormals[i]);
        aiVector3D* texture = nullptr;// (mesh->mTextureCoords[i]);
        //g_Vertices[i] = Vertex(vert->x, vert->y, vert->z, 0.5f, 0.5f ,normal->x, normal->y, normal->z);
        renderSys.vertices.push_back(Vertex(vert->x, vert->y, vert->z, 0.5f, 0.5f, normal->x, normal->y, normal->z));
    }

    numFaces = mesh->mNumFaces;
    g_Indices = new std::uint16_t[numFaces * 3];
    for (int i = 0; i < numFaces; i++) {
        aiFace *face = &(mesh->mFaces[i]);
        for (int j = 0; j < face->mNumIndices; j++) {
            g_Indices[i * 3 + j] = face->mIndices[j];
        }

    }


    //----------------------------Skybox----------------------
    wrl::ComPtr<ID3D11Resource> skyTexture;
    hr = dx::CreateDDSTextureFromFileEx(g_d3dDevice.Get(), L"./shaders/skybox2.dds", 0, D3D11_USAGE_DEFAULT,
        D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_TEXTURECUBE, dx::DX11::DDS_LOADER_DEFAULT, skyTexture.GetAddressOf(), cubemapView.GetAddressOf());
    RETURN_IF_FAILED(hr);


    wrl::ComPtr<ID3DBlob> verShaderBlob2;
    wrl::ComPtr<ID3DBlob> pixShaderBlob2;

    hr = LoadShader<ID3D11VertexShader>(L"./shaders/SkyboxVertexShader.hlsl", "vs", verShaderBlob2.GetAddressOf(), g_d3dVertexShaderSkybox.GetAddressOf(), g_d3dDevice.Get());
    RETURN_IF_FAILED(hr);

    hr = LoadShader<ID3D11PixelShader>(L"./shaders/SkyboxPixelShader.hlsl", "ps", pixShaderBlob2.GetAddressOf(), g_d3dPixelShaderSkybox.GetAddressOf(), g_d3dDevice.Get());
    RETURN_IF_FAILED(hr);

    D3D11_BUFFER_DESC cubeBuffDesc;
    ZeroMemory(&cubeBuffDesc, sizeof(cubeBuffDesc));
    cubeBuffDesc.ByteWidth = sizeof(g_Vertices2);
    cubeBuffDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    cubeBuffDesc.Usage = D3D11_USAGE_DEFAULT;

    D3D11_SUBRESOURCE_DATA cubeData;
    cubeData.pSysMem = g_Vertices2;
    cubeData.SysMemPitch = 0;
    cubeData.SysMemSlicePitch = 0;

    hr = g_d3dDevice->CreateBuffer(&cubeBuffDesc, &cubeData, g_d3dVertexBufferSkybox.GetAddressOf());
    RETURN_IF_FAILED(hr);

    D3D11_BUFFER_DESC cubeIndexBuffDesc;
    ZeroMemory(&cubeIndexBuffDesc, sizeof(cubeIndexBuffDesc));
    cubeIndexBuffDesc.ByteWidth = sizeof(g_Indices2);
    cubeIndexBuffDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    cubeIndexBuffDesc.Usage = D3D11_USAGE_DEFAULT;
    
    D3D11_SUBRESOURCE_DATA cubeIndexData;
    cubeIndexData.pSysMem = g_Indices2;
    cubeIndexData.SysMemPitch = 0;
    cubeIndexData.SysMemSlicePitch = 0;

    hr = g_d3dDevice->CreateBuffer(&cubeIndexBuffDesc, &cubeIndexData, g_d3dIndexBufferSkybox.GetAddressOf());
    RETURN_IF_FAILED(hr);

    D3D11_RASTERIZER_DESC rastSkyboxDesc;
    ZeroMemory(&rastSkyboxDesc, sizeof(rastSkyboxDesc));
    rastSkyboxDesc.CullMode = D3D11_CULL_NONE;
    rastSkyboxDesc.FillMode = D3D11_FILL_SOLID;
    rastSkyboxDesc.FrontCounterClockwise = false;
    rastSkyboxDesc.AntialiasedLineEnable = false;
    rastSkyboxDesc.MultisampleEnable = false;
    
    hr = g_d3dDevice->CreateRasterizerState(&rastSkyboxDesc, g_d3dRasterizerStateSkybox.GetAddressOf());
    RETURN_IF_FAILED(hr);

    //------------------------------------------------------

    wrl::ComPtr<ID3D11InputLayout> vertLayout ;

    wrl::ComPtr<ID3DBlob> verShaderBlob ;
    wrl::ComPtr<ID3DBlob> pixShaderBlob;
    

    hr = LoadShader<ID3D11VertexShader>(L"./shaders/BasicVertexShader.hlsl", "vs", verShaderBlob.GetAddressOf(), g_d3dVertexShader.GetAddressOf(), g_d3dDevice.Get());
    RETURN_IF_FAILED(hr);

    hr = LoadShader<ID3D11PixelShader>(L"./shaders/BasicPixelShader.hlsl", "ps", pixShaderBlob.GetAddressOf(), g_d3dPixelShader.GetAddressOf(), g_d3dDevice.Get());
    RETURN_IF_FAILED(hr);

    D3D11_BUFFER_DESC vertBuffDesc;
    vertBuffDesc.Usage = D3D11_USAGE_DEFAULT;
    vertBuffDesc.ByteWidth = sizeof(Vertex) * numVertices;
    vertBuffDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertBuffDesc.CPUAccessFlags = 0;
    vertBuffDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA vertSubresource;
    vertSubresource.pSysMem = &(renderSys.vertices[0]);
    vertSubresource.SysMemPitch = 0;
    vertSubresource.SysMemSlicePitch = 0;

    hr = g_d3dDevice->CreateBuffer(&vertBuffDesc, &vertSubresource, g_d3dVertexBuffer.GetAddressOf());
    RETURN_IF_FAILED(hr);

    std::uint32_t strides[] = { sizeof(Vertex) };
    std::uint32_t offsets[] = { 0 };
    g_d3dDeviceContext->IASetVertexBuffers(0, 1, g_d3dVertexBuffer.GetAddressOf(), strides, offsets);

    D3D11_BUFFER_DESC indxBuffDesc;
    indxBuffDesc.Usage = D3D11_USAGE_DEFAULT;
    indxBuffDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indxBuffDesc.CPUAccessFlags = 0;
    indxBuffDesc.ByteWidth = sizeof(std::uint16_t) * numFaces * 3;
    indxBuffDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA indxSubresource;
    indxSubresource.pSysMem = g_Indices;
    indxSubresource.SysMemPitch = 0;
    indxSubresource.SysMemSlicePitch = 0;

    hr = g_d3dDevice->CreateBuffer(&indxBuffDesc, &indxSubresource, g_d3dIndexBuffer.GetAddressOf());
    RETURN_IF_FAILED(hr);

    g_d3dDeviceContext->IASetIndexBuffer(g_d3dIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

    g_d3dDeviceContext->VSSetShader(g_d3dVertexShader.Get(), 0, 0);
    g_d3dDeviceContext->PSSetShader(g_d3dPixelShader.Get(), 0, 0);

    hr = g_d3dDevice->CreateInputLayout(vertexLayouts, 3, verShaderBlob->GetBufferPointer(), verShaderBlob->GetBufferSize(), vertLayout.GetAddressOf());
    RETURN_IF_FAILED(hr);

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
    RETURN_IF_FAILED(hr);

    hr = g_d3dDevice->CreateDepthStencilView(g_d3dDepthStencilBuffer.Get(), NULL, g_d3dDepthStencilView.GetAddressOf());
    RETURN_IF_FAILED(hr);

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
    RETURN_IF_FAILED(hr);

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
    RETURN_IF_FAILED(hr);

    g_d3dDeviceContext->VSSetConstantBuffers(1, 1, g_cbPerCam.GetAddressOf());

    //g_projMatrix = dx::XMMatrixPerspectiveFovLH(0.4f * 3.14f, g_Viewport.Width / g_Viewport.Height, 1.0f, 100.0f);

    ProjCB projCB;
    projCB.projMatrix = g_cam.getProjMatrix();

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
    RETURN_IF_FAILED(hr);

    g_d3dDeviceContext->VSSetConstantBuffers(2, 1, g_cbPerProj.GetAddressOf());

    lightCB.dLight.direction = dx::XMVector4Transform(dLight.direction, g_cam.getViewMatrix());
    lightCB.pointLights[0].pos = dx::XMVector4Transform(pLight1.pos, g_cam.getViewMatrix());
    lightCB.pointLights[0].constant = pLight1.constant;
    lightCB.pointLights[0].linear = pLight1.linear;
    lightCB.pointLights[0].quadratic = pLight1.quadratic;

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
    rastDesc.AntialiasedLineEnable = false;
    rastDesc.MultisampleEnable = false;
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

    
    hr = dx::CreateWICTextureFromFile(g_d3dDevice.Get(), L"./shaders/glass.png", NULL, textureView.GetAddressOf());
    RETURN_IF_FAILED(hr);

    D3D11_DEPTH_STENCIL_DESC depthDesc;
    ZeroMemory(&depthDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
    depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    depthDesc.DepthEnable = true;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    
    hr = g_d3dDevice->CreateDepthStencilState(&depthDesc, g_d3dDepthStencilStateSkybox.GetAddressOf());
    RETURN_IF_FAILED(hr);


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

    hr = g_d3dDevice->CreateSamplerState(&samplerDesc, samplerState.GetAddressOf());
    RETURN_IF_FAILED(hr);

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

    //D3D11_SAMPLER_DESC samplerSkyDesc;
    //ZeroMemory(&samplerSkyDesc, sizeof(samplerSkyDesc));
    //samplerSkyDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    //samplerSkyDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    //samplerSkyDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    //samplerSkyDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    //samplerSkyDesc.MipLODBias = 0.0f;
    //samplerSkyDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    //samplerSkyDesc.MinLOD = 0.0f;
    //samplerSkyDesc.MaxLOD = FLT_MAX;
    //samplerSkyDesc.MaxAnisotropy = 16;
    //
    //wrl::ComPtr<ID3D11SamplerState> samplerSkyState;
    //g_d3dDevice->CreateSamplerState(&samplerSkyDesc, samplerSkyState.GetAddressOf());

    //g_d3dDeviceContext->PSSetSamplers(0, 1, samplerSkyState.GetAddressOf());

    

    std::uint32_t strides[] = { sizeof(Vertex) };
    std::uint32_t offsets[] = { 0 };

    g_cam.rotate(0.0f, -0.00f, 0.0f);
    viewCB.viewMatrix = g_cam.getViewMatrix();
    g_d3dDeviceContext->UpdateSubresource(g_cbPerCam.Get(), 0, NULL, &viewCB, 0, 0);
    float blendFactor[4] = { 0.5f, 0.5f, 0.5f, 0.5f };


    //Update light direction with view matrix
    lightCB.dLight.direction = dx::XMVector4Transform(dLight.direction, g_cam._viewMatrix);
    lightCB.pointLights[0].pos = dx::XMVector4Transform(pLight1.pos, g_cam._viewMatrix);
    g_d3dDeviceContext->UpdateSubresource(g_cbLight.Get(), 0, NULL, &lightCB, 0, 0);

    //g_d3dDeviceContexOMSetBlendState(g_d3dBlendState, blendFactor, 0xffffffff);
    
    dx::XMMATRIX transMat = dx::XMMatrixTranslationFromVector(g_cam._camPos);

    objCB.worldMatrix = dx::XMMatrixTranspose(transMat);
    g_d3dDeviceContext->UpdateSubresource(g_cbPerObj.Get(), 0, NULL, &objCB, 0, 0);

    g_d3dDeviceContext->IASetVertexBuffers(0, 1, g_d3dVertexBufferSkybox.GetAddressOf(), strides, offsets);
    g_d3dDeviceContext->IASetIndexBuffer(g_d3dIndexBufferSkybox.Get(), DXGI_FORMAT_R16_UINT, 0);

    g_d3dDeviceContext->VSSetShader(g_d3dVertexShaderSkybox.Get(), 0, 0);
    g_d3dDeviceContext->PSSetShader(g_d3dPixelShaderSkybox.Get(), 0, 0);

    g_d3dDeviceContext->PSSetShaderResources(0, 1, cubemapView.GetAddressOf());

    g_d3dDeviceContext->OMSetDepthStencilState(g_d3dDepthStencilStateSkybox.Get(), 0);
    g_d3dDeviceContext->RSSetState(g_d3dRasterizerStateSkybox.Get());
    g_d3dDeviceContext->DrawIndexed(36, 0, 0);

    //models
    g_d3dDeviceContext->OMSetDepthStencilState(NULL, 0);

    transMat = dx::XMMatrixTranslation(0.0f, 0.0f, 5.0f);
    objCB.worldMatrix = dx::XMMatrixTranspose(dx::XMMatrixRotationAxis(dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rot) * scaleMat * transMat);
    g_d3dDeviceContext->UpdateSubresource(g_cbPerObj.Get(), 0, NULL, &objCB, 0, 0);
    
    g_d3dDeviceContext->IASetVertexBuffers(0, 1, g_d3dVertexBuffer.GetAddressOf(), strides, offsets);
    g_d3dDeviceContext->IASetIndexBuffer(g_d3dIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    g_d3dDeviceContext->VSSetShader(g_d3dVertexShader.Get(), 0, 0);
    g_d3dDeviceContext->PSSetShader(g_d3dPixelShader.Get(), 0, 0);

    g_d3dDeviceContext->PSSetShaderResources(0, 1, textureView.GetAddressOf());

    g_d3dDeviceContext->RSSetState(g_d3dRasterizerStateCullFront.Get());
    g_d3dDeviceContext->DrawIndexed(numFaces * 3, 0, 0);
    g_d3dDeviceContext->RSSetState(g_d3dRasterizerStateCullBack.Get());
    g_d3dDeviceContext->DrawIndexed(numFaces * 3, 0, 0);

    transMat = dx::XMMatrixTranslation(-3.0f, -0.5f, 4.0f);
    objCB.worldMatrix = dx::XMMatrixTranspose(dx::XMMatrixRotationAxis(dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rot)* scaleMat * transMat);
    g_d3dDeviceContext->UpdateSubresource(g_cbPerObj.Get(), 0, NULL, &objCB, 0, 0);

    g_d3dDeviceContext->RSSetState(g_d3dRasterizerStateCullFront.Get());
    g_d3dDeviceContext->DrawIndexed(numFaces * 3, 0, 0);
    g_d3dDeviceContext->RSSetState(g_d3dRasterizerStateCullBack.Get());
    g_d3dDeviceContext->DrawIndexed(numFaces * 3, 0, 0);

    ImGui::Text("mouse X: %d \nmouse Y: %d ", mouse.relX, mouse.relY);

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    g_dxgiSwapChain->Present(1, 0);

}

void Update(float deltaTime) {
    rot += (0.5f * deltaTime);
    if (rot > 6.28f) {
        rot = 0.0f;
    }
    //rot = 0.0f;
}

void Cleanup() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    delete[] g_Indices;

    if (g_d3dDeviceContext) {
        g_d3dDeviceContext->ClearState();
        g_d3dDeviceContext->Flush();
    }
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
    

    //_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    //_CrtDumpMemoryLeaks();


    return returnCode;
}