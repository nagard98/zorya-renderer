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

#include "Camera.h"
#include "Lights.h"
#include "Shaders.h"
#include "Model.h"
#include "RHIState.h"
#include "RenderHardwareInterface.h"
#include "RendererFrontend.h"
#include "RendererBackend.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")
#pragma comment (lib, "winmm.lib")

#define RETURN_IF_FAILED(hResult) { if(FAILED(hResult)) return hResult; }

namespace dx = DirectX;
namespace wrl = Microsoft::WRL;

const LONG g_windowWidth = 1280;
const LONG g_windowHeight = 720;
LPCSTR g_windowClassName = "DirectXWindowClass";
LPCSTR g_windowName = "Derma";
HWND g_windowHandle = 0;

const BOOL g_enableVSync = TRUE;

Assimp::Importer importer;


wrl::ComPtr<ID3D11ShaderResourceView> cubemapView;

wrl::ComPtr<ID3D11DepthStencilState> g_d3dDepthStencilStateSkybox;

wrl::ComPtr<ID3D11InputLayout> g_d3dInputLayout;

wrl::ComPtr<ID3D11Buffer> g_d3dVertexBufferSkybox;
wrl::ComPtr<ID3D11Buffer> g_d3dIndexBufferSkybox;

wrl::ComPtr<ID3D11Buffer> g_cbPerObj;
wrl::ComPtr<ID3D11Buffer> g_cbPerCam;
wrl::ComPtr<ID3D11Buffer> g_cbPerProj;
wrl::ComPtr<ID3D11Buffer> g_cbLight;

wrl::ComPtr<ID3D11VertexShader> g_d3dVertexShader;

wrl::ComPtr<ID3D11PixelShader> g_d3dPixelShaderSkybox;
wrl::ComPtr<ID3D11VertexShader> g_d3dVertexShaderSkybox;

wrl::ComPtr<ID3D11SamplerState> samplerStateCube;

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
    {"normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"tangent", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
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


DirectionalLight dLight = { dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f) };
PointLight pLight1 = { dx::XMVectorSet(2.0f, 2.0f, 0.0f, 1.0f), 1.0f, 0.22f, 0.20f };

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
    return rhi.Init(g_windowHandle);
}


HRESULT LoadSkybox(const wchar_t *skyboxPath) {
    HRESULT hr;

    //----------------------------Skybox----------------------
    wrl::ComPtr<ID3D11Resource> skyTexture;
    hr = dx::CreateDDSTextureFromFileEx(rhi.device.Get(), skyboxPath, 0, D3D11_USAGE_DEFAULT,
        D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_TEXTURECUBE, dx::DX11::DDS_LOADER_DEFAULT, skyTexture.GetAddressOf(), cubemapView.GetAddressOf());
    RETURN_IF_FAILED(hr);

    wrl::ComPtr<ID3DBlob> verShaderBlob2;
    wrl::ComPtr<ID3DBlob> pixShaderBlob2;

    hr = LoadShader<ID3D11VertexShader>(L"./shaders/SkyboxVertexShader.hlsl", "vs", verShaderBlob2.GetAddressOf(), g_d3dVertexShaderSkybox.GetAddressOf(), rhi.device.Get());
    RETURN_IF_FAILED(hr);

    hr = LoadShader<ID3D11PixelShader>(L"./shaders/SkyboxPixelShader.hlsl", "ps", pixShaderBlob2.GetAddressOf(), g_d3dPixelShaderSkybox.GetAddressOf(), rhi.device.Get());
    RETURN_IF_FAILED(hr);

    D3D11_BUFFER_DESC cubeBuffDesc;
    ZeroMemory(&cubeBuffDesc, sizeof(cubeBuffDesc));
    cubeBuffDesc.ByteWidth = sizeof(Vertex) * cubeVertices.size();
    cubeBuffDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    cubeBuffDesc.Usage = D3D11_USAGE_DEFAULT;

    D3D11_SUBRESOURCE_DATA cubeData;
    cubeData.pSysMem = cubeVertices.data();
    cubeData.SysMemPitch = 0;
    cubeData.SysMemSlicePitch = 0;

    hr = rhi.device->CreateBuffer(&cubeBuffDesc, &cubeData, g_d3dVertexBufferSkybox.GetAddressOf());
    RETURN_IF_FAILED(hr);

    D3D11_BUFFER_DESC cubeIndexBuffDesc;
    ZeroMemory(&cubeIndexBuffDesc, sizeof(cubeIndexBuffDesc));
    cubeIndexBuffDesc.ByteWidth = sizeof(std::uint16_t) * cubeIndices.size();
    cubeIndexBuffDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    cubeIndexBuffDesc.Usage = D3D11_USAGE_DEFAULT;

    D3D11_SUBRESOURCE_DATA cubeIndexData;
    cubeIndexData.pSysMem = cubeIndices.data();
    cubeIndexData.SysMemPitch = 0;
    cubeIndexData.SysMemSlicePitch = 0;

    hr = rhi.device->CreateBuffer(&cubeIndexBuffDesc, &cubeIndexData, g_d3dIndexBufferSkybox.GetAddressOf());

    return hr;
    //---------------------------------------------------------------------
}


dx::XMMATRIX scaleMat = scaleMat = dx::XMMatrixScaling(/*sM.a1, sM.b2, sM.c3*/11.0f, 11.0f, 11.0f);
ViewDesc vDesc;

HRESULT InitData() {

    HRESULT hr;
    rf.InitScene();
    rb.Init();

    hr = LoadSkybox(L"./shaders/assets/skybox_space.dds");
    RETURN_IF_FAILED(hr);

    ModelHandle_t mHnd = rf.LoadModelFromFile("./shaders/assets/Human/Models/Head/Head.fbx");//Human/Models/Head/Head.fbx");

    wrl::ComPtr<ID3DBlob> verShaderBlob ;
    hr = LoadShader<ID3D11VertexShader>(L"./shaders/BasicVertexShader.hlsl", "vs", verShaderBlob.GetAddressOf(), g_d3dVertexShader.GetAddressOf(), rhi.device.Get());
    RETURN_IF_FAILED(hr);

    rhi.context->VSSetShader(g_d3dVertexShader.Get(), 0, 0);

    wrl::ComPtr<ID3D11InputLayout> vertLayout;
    hr = rhi.device->CreateInputLayout(vertexLayouts, 4, verShaderBlob->GetBufferPointer(), verShaderBlob->GetBufferSize(), vertLayout.GetAddressOf());
    RETURN_IF_FAILED(hr);

    rhi.context->IASetInputLayout(vertLayout.Get());
    rhi.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


    //World transform constant buffer setup---------------------------------------------------
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

    hr = rhi.device->CreateBuffer(&cbObjDesc, &resourceCbPerObj, g_cbPerObj.GetAddressOf());
    RETURN_IF_FAILED(hr);

    rhi.context->VSSetConstantBuffers(0, 1, g_cbPerObj.GetAddressOf());
    //---------------------------------------------------


    //View matrix constant buffer setup-------------------------------------------------------------
    dx::XMVECTOR camPos = dx::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    dx::XMVECTOR camDir = dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    dx::XMVECTOR camUp = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    g_cam = Camera(camPos, camDir, camUp, 60.0f * (3.14f/180.0f), rhi.viewport.Width / rhi.viewport.Height, 1.0f, 100.0f);

    viewCB.viewMatrix = g_cam.getViewMatrix();

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


    hr = rhi.device->CreateBuffer(&cbCamDesc, &resourceCbPerCam, g_cbPerCam.GetAddressOf());
    RETURN_IF_FAILED(hr);

    rhi.context->VSSetConstantBuffers(1, 1, g_cbPerCam.GetAddressOf());
    //----------------------------------------------------------------


    //Projection matrix constant buffer setup--------------------------------------
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


    hr = rhi.device->CreateBuffer(&cbProjDesc, &resourceCbPerProj, g_cbPerProj.GetAddressOf());
    RETURN_IF_FAILED(hr);

    rhi.context->VSSetConstantBuffers(2, 1, g_cbPerProj.GetAddressOf());
    //------------------------------------------------------------------

    //Light constant buffer setup---------------------------------------
    lightCB.dLight.direction = dx::XMVector4Transform(dLight.direction, g_cam.getViewMatrix());
    lightCB.pointLights[0].pos = dx::XMVector4Transform(pLight1.pos, g_cam.getViewMatrix());
    lightCB.pointLights[0].constant = pLight1.constant;
    lightCB.pointLights[0].linear = pLight1.linear;
    lightCB.pointLights[0].quadratic = pLight1.quadratic;
    lightCB.numPLights = 0;

    D3D11_SUBRESOURCE_DATA lightResource;
    lightResource.pSysMem = &lightCB;
    lightResource.SysMemPitch = 0;
    lightResource.SysMemSlicePitch = 0;

    D3D11_BUFFER_DESC lightBuffDesc;
    ZeroMemory(&lightBuffDesc, sizeof(lightBuffDesc));
    lightBuffDesc.Usage = D3D11_USAGE_DEFAULT;
    lightBuffDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    lightBuffDesc.ByteWidth = sizeof(lightCB);
    
    rhi.device->CreateBuffer(&lightBuffDesc, &lightResource, g_cbLight.GetAddressOf());
    rhi.context->PSSetConstantBuffers(0, 1, g_cbLight.GetAddressOf());
    //---------------------------------------------------------


    //Vertex v(1.0f, 1.0f, 2.0f, 3.0f, 1.0f, 2.0f, 2.3, 4.4f, 1.2f, 1.2, 3.5f);
    //std::ofstream of("test.dscene", std::ios::out | std::ios::binary);
    //if (!of) {
    //    std::cout << "failed to openfile" << std::endl;
    //    return E_ACCESSDENIED;
    //}

    //of.write((char*)&v, sizeof(Vertex));
    //of.close();
    //
    //std::ifstream ifs("test.dscene", std::ios::out | std::ios::binary);
    //if (!ifs) {
    //    std::cout << "failed to open file" << std::endl;
    //    return E_ACCESSDENIED;
    //}

    //Vertex v2(0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f);
    //ifs.read((char*)&v2, sizeof(Vertex));
    //ifs.close();

    return hr;
}

float rot = 0.0f;
float smoothness = 0.5f;

void Render() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();

    rhi.context->ClearRenderTargetView(rhi.renderTargetView.Get(), dx::Colors::Black);
    rhi.context->ClearDepthStencilView(rhi.depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);


    std::uint32_t strides[] = { sizeof(Vertex) };
    std::uint32_t offsets[] = { 0 };

    g_cam.rotate(0.0f, -0.00f, 0.0f);
    viewCB.viewMatrix = g_cam.getViewMatrix();
    rhi.context->UpdateSubresource(g_cbPerCam.Get(), 0, NULL, &viewCB, 0, 0);
    float blendFactor[4] = { 0.5f, 0.5f, 0.5f, 0.5f };

    //Update light direction with view matrix
    lightCB.dLight.direction = dx::XMVector4Transform(dLight.direction, g_cam._viewMatrix);
    lightCB.pointLights[0].pos = dx::XMVector4Transform(pLight1.pos, g_cam._viewMatrix);
    rhi.context->UpdateSubresource(g_cbLight.Get(), 0, NULL, &lightCB, 0, 0);

    //g_d3dDeviceContexOMSetBlendState(g_d3dBlendState, blendFactor, 0xffffffff);
    
    dx::XMMATRIX transMat = dx::XMMatrixTranslationFromVector(g_cam._camPos);

    objCB.worldMatrix = dx::XMMatrixTranspose(transMat);
    rhi.context->UpdateSubresource(g_cbPerObj.Get(), 0, NULL, &objCB, 0, 0);

    //skybox-----------------------------------------------------------
    rhi.context->IASetVertexBuffers(0, 1, g_d3dVertexBufferSkybox.GetAddressOf(), strides, offsets);
    rhi.context->IASetIndexBuffer(g_d3dIndexBufferSkybox.Get(), DXGI_FORMAT_R16_UINT, 0);

    rhi.context->VSSetShader(g_d3dVertexShaderSkybox.Get(), 0, 0);
    rhi.context->PSSetShader(g_d3dPixelShaderSkybox.Get(), 0, 0);

    rhi.context->PSSetShaderResources(0, 1, cubemapView.GetAddressOf());
    
    RHI_OM_DS_SET_DEPTH_COMP_LESS_EQ(rhiState);
    RHI_RS_SET_CULL_FRONT(rhiState);
    rhi.SetState(rhiState);
    rhi.context->DrawIndexed(36, 0, 0);
    //---------------------------------------------------------------


    //models-----------------------------------------------------
    transMat = dx::XMMatrixTranslation(0.0f, -3.0f, 5.0f);
    objCB.worldMatrix = dx::XMMatrixTranspose(dx::XMMatrixRotationAxis(dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rot) * scaleMat * transMat);
    rhi.context->UpdateSubresource(g_cbPerObj.Get(), 0, NULL, &objCB, 0, 0);
    
    rhi.context->VSSetShader(g_d3dVertexShader.Get(), 0, 0);
    
    vDesc = rf.ComputeView();

    ImGui::SliderFloat("smoothness", &smoothness, 0.0f, 1.0f);
    //TODO: remove parameter smoothness from RenderView; used only for initial development testing
    rb.RenderView(vDesc, smoothness);
    //-------------------------------------------------------------

    ImGui::Text("mouse X: %d \nmouse Y: %d ", mouse.relX, mouse.relY);

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    rhi.swapChain->Present(g_enableVSync, 0);

}

void Update(float deltaTime) {
    rot += (0.3f * deltaTime);
    if (rot > 6.28f) {
        rot = 0.0f;
    }
    rot = 0.0f;
}

void Cleanup() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    //delete[] g_Indices;

    if (rhi.context) {
        rhi.context->ClearState();
        rhi.context->Flush();
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