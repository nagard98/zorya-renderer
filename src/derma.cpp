#define _CRTDBG_MAP_ALLOC

#include <stdlib.h>
#include <crtdbg.h>
#include <Windows.h>
#include <hidusage.h>

#include <d3d11_1.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <DirectXColors.h>

#include <wrl/client.h>

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include <Editor/Logger.h>

#include "Camera.h"
#include "Shaders.h"
#include "Model.h"
#include "RHIState.h"
#include "RenderHardwareInterface.h"
#include "RendererFrontend.h"
#include "RendererBackend.h"
#include "ApplicationConfig.h"
#include <Editor/Editor.h>
#include "SceneGraph.h"

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

HWND g_windowHandle = 0;

Assimp::Importer importer;
Editor editor;

ID3D11Debug* g_debugLayer;

Camera g_cam;



struct Mouse {
    std::int64_t relX, relY;
};

Mouse mouse;

//-----------------------------------------------------

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void Update(float deltaTime);
void Render();
void Cleanup();
HRESULT InitData();

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

    RECT windowRect = { 0,0,g_resolutionWidth,g_resolutionHeight };
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
    case WM_SIZE : {
        HRESULT hr = rhi.ResizeWindow(LOWORD(lParam), HIWORD(lParam));
        if (hr != S_FALSE && !FAILED(hr)) {
            //rb.Init(true);
        }
        break;
    }
    case WM_PAINT:
        BeginPaint(hwnd, &paintStruct);
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

    HRESULT hr = rhi.Init(g_windowHandle);

    //TODO: move gui initialization elsewhere
    if (!FAILED(hr)) {
        ImGui_ImplDX11_Init(rhi.device.device, rhi.context);
    }

    return hr;
}

dx::XMMATRIX scaleMat = dx::XMMatrixScaling(1.0f, 1.0f, 1.0f);

HRESULT InitData() {

    HRESULT hr;
    
    hr = rb.Init();
    RETURN_IF_FAILED(hr);

    hr = shaders.Init();
    RETURN_IF_FAILED(hr);  

    //RenderableEntity mHnd4 = rf.LoadModelFromFile("./shaders/assets/nissan/source/nissan2.obj");
    //RenderableEntity mHnd5 = rf.LoadModelFromFile("./shaders/assets/cornell/cornell.fbx");
    //RenderableEntity mHnd = rf.LoadModelFromFile("./shaders/assets/perry/head.obj");
    //RenderableEntity mHnd2 = rf.LoadModelFromFile("./shaders/assets/cicada/source/cicada.fbx");
    //RenderableEntity mHnd3 = rf.LoadModelFromFile("./shaders/assets/Human/Models/Head/Head.fbx");
    //RenderableEntity mHnd6 = rf.LoadModelFromFile("./shaders/assets/cube.dae");
    //RenderableEntity mHnd7 = rf.LoadModelFromFile("./shaders/assets/nile/source/nile.obj");
    //RenderableEntity mHnd8 = rf.LoadModelFromFile("./shaders/assets/nixdorf/scene.gltf");
    //RenderableEntity mHnd9 = rf.LoadModelFromFile("./shaders/assets/ye-gameboy/scene.gltf", true);
    //RenderableEntity mHnd10 = rf.LoadModelFromFile("./shaders/assets/old_tv/scene.gltf", true);
    //RenderableEntity mHnd11 = rf.LoadModelFromFile("./shaders/assets/sphere.obj", true);
    //RenderableEntity mHnd12 = rf.LoadModelFromFile("./shaders/assets/mori_knob/testObj.obj", true);
    //RenderableEntity mHnd13 = rf.LoadModelFromFile("./shaders/assets/cl-gameboy-gltf/scene.gltf", true);
    //RenderableEntity mHnd14 = rf.LoadModelFromFile("./shaders/assets/cl-gameboy-fbx/source/GameBoy_low_01_Fbx.fbx");
    //RenderableEntity mHnd15 = rf.LoadModelFromFile("./shaders/assets/plane.obj", true);
    //RenderableEntity mHnd16 = rf.LoadModelFromFile("./shaders/assets/cornell-box/CornellBox-Original.obj");
    //RenderableEntity mHnd17 = rf.LoadModelFromFile("./shaders/assets/sphere.dae");
    RenderableEntity mHnd18 = rf.LoadModelFromFile("./shaders/assets/brocc-the-athlete/source/Sporter_Retopo.fbx");
    rf.AddLight(nullptr, dx::XMVectorSet(1.0f, 0.0f, 0.0, 0.0f), 1.0f, 8.0f);
    //rf.AddLight(nullptr, dx::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0.22f, 0.20f);

    rhi.context->IASetInputLayout(shaders.vertexLayout);
    rhi.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


    ////Camera setup-------------------------------------------------------------
    dx::XMVECTOR camPos = dx::XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f);
    dx::XMVECTOR camDir = dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    dx::XMVECTOR camUp = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    g_cam = Camera(camPos, camDir, camUp, dx::XM_PIDIV2 * rhi.viewport.Height / rhi.viewport.Width, rhi.viewport.Width / rhi.viewport.Height, 0.05f, 100.0f);
    //-----------------------------------------------------------------------------

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

void Render() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();

    rhi.context->ClearRenderTargetView(rb.finalRenderTargetView, dx::Colors::Black);
    rhi.context->ClearDepthStencilView(rb.depthDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    rhi.context->OMSetRenderTargets(1, &rb.finalRenderTargetView, rb.depthDSV);
    rhi.context->RSSetViewports(1, &rb.sceneViewport);

    //g_cam.rotateAroundCamAxis(0.0f, -0.00f, 0.0f);    

    // Rendering cam view -----------------------------------------------------    
    const ViewDesc vDesc = rf.ComputeView(g_cam);
    rb.RenderView(vDesc);
    //-------------------------------------------------------------

    rhi.context->ClearRenderTargetView(rhi.backBufferRTV, dx::Colors::Black);
    rhi.context->ClearDepthStencilView(rhi.backBufferDepthDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    rhi.context->OMSetRenderTargets(1, &rhi.backBufferRTV, rhi.backBufferDepthDSV);
    rhi.context->RSSetViewports(1, &rhi.viewport);

    rb.annot->BeginEvent(L"Editor Pass");
    {
        //rhi.context->ResolveSubresource(rhi.editorSceneTex, 0, rtTexture, 0, DXGI_FORMAT_R8G8B8A8_UNORM);

        editor.RenderEditor(rf, g_cam, rb.finalRenderTargetSRV);
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
    rb.annot->EndEvent();

    //rhi.context->OMSetRenderTargets(1, &rhi.backBufferRTV, nullptr);
    //rhi.context->VSSetShader(shaders.vertexShaders.at((std::uint8_t)VShaderID::FULL_QUAD), nullptr, 0);
    //rhi.context->PSSetShader(shaders.pixelShaders.at((std::uint8_t)PShaderID::PRESENT), nullptr, 0);

    //rhi.context->PSSetShaderResources(0, 1, &rb.finalRenderTargetSRV);
    //rhi.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    //rhi.context->Draw(4, 0);
    //rhi.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //ID3D11ShaderResourceView* nullSRV[] = { nullptr };
    //rhi.context->PSSetShaderResources(0, 1, nullSRV);

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

    bufferCache.ReleaseAllResources();
    resourceCache.ReleaseAllResources();
    shaders.ReleaseAllResources();
    rb.ReleaseAllResources();
    rhi.ReleaseAllResources();

    rhi.context->ClearState();
    rhi.context->Flush();

    if (rhi.context)rhi.context->Release();
    if (rhi.device.device) rhi.device.device->Release();

    if (g_debugLayer) {
        g_debugLayer->ReportLiveDeviceObjects(D3D11_RLDO_FLAGS::D3D11_RLDO_DETAIL);
        g_debugLayer->Release();
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

#ifdef _DEBUG
    if (FAILED(rhi.device.device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&g_debugLayer)))) {
        MessageBox(NULL, "Failed to initialize correctly d3d debug layer", "Error", MB_OK);
        Cleanup();
        return -1;
    }
#endif // _DEBUG

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