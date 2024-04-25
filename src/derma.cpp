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
#include "imgui_impl_win32.h"

#include "editor/Logger.h"
#include "editor/Editor.h"

#include "renderer/frontend/Camera.h"
#include "renderer/frontend/Model.h"
#include "renderer/frontend/SceneGraph.h"
#include "renderer/frontend/RendererFrontend.h"
#include "renderer/frontend/Shader.h"

#include "renderer/backend/rhi/RHIState.h"
#include "renderer/backend/rhi/RenderHardwareInterface.h"
#include "renderer/backend/RendererBackend.h"

#include "ApplicationConfig.h"

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

#define RETURN_IF_FAILED(h_result) { if(FAILED(h_result)) return h_result; }

namespace dx = DirectX;
namespace wrl = Microsoft::WRL;

HWND g_window_handle = 0;

Assimp::Importer m_importer;
zorya::Editor editor;

ID3D11Debug* g_debug_layer;

Camera g_cam;

struct Mouse
{
	std::int64_t rel_x, rel_y;
};

Mouse mouse;

//-----------------------------------------------------

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void update(float delta_time);
void render();
void cleanup();
HRESULT init_data();

int init_application(HINSTANCE hInstance, int cmd_show)
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;


	WNDCLASSEX wnd_class_ex = { 0 };
	wnd_class_ex.cbSize = sizeof(WNDCLASSEX);
	wnd_class_ex.style = CS_HREDRAW | CS_VREDRAW;
	wnd_class_ex.lpfnWndProc = &WndProc;
	wnd_class_ex.hInstance = hInstance;
	wnd_class_ex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wnd_class_ex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wnd_class_ex.lpszClassName = g_windowClassName;
	wnd_class_ex.lpszMenuName = nullptr;

	if (!RegisterClassEx(&wnd_class_ex))
	{
		return -1;
	}

	RECT window_rect = { 0,0,g_resolutionWidth,g_resolutionHeight };
	AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, FALSE);

	g_window_handle = CreateWindow(
		g_windowClassName,
		g_windowName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		window_rect.right - window_rect.left,
		window_rect.bottom - window_rect.top,
		nullptr, nullptr, hInstance, nullptr);

	if (!g_window_handle)
	{
		return -1;
	}

	RAWINPUTDEVICE rid[1];
	rid[0].hwndTarget = g_window_handle;
	rid[0].dwFlags = RIDEV_INPUTSINK;
	rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
	rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;

	RegisterRawInputDevices(rid, 1, sizeof(rid[0]));

	ImGui_ImplWin32_Init(g_window_handle);

	ShowWindow(g_window_handle, cmd_show);
	UpdateWindow(g_window_handle);

	return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT paint_struct;

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

		if (raw->header.dwType == RIM_TYPEMOUSE)
		{
			mouse.rel_x = raw->data.mouse.lLastX;
			mouse.rel_y = raw->data.mouse.lLastY;
		}

		break;
	}
	case WM_SIZE:
	{
		HRESULT hr = rhi.resize_window(LOWORD(lParam), HIWORD(lParam));
		if (hr != S_FALSE && !FAILED(hr))
		{
			//rb.Init(true);
		}
		break;
	}
	case WM_PAINT:
		BeginPaint(hwnd, &paint_struct);
		EndPaint(hwnd, &paint_struct);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return LRESULT();
}

int run()
{

	MSG msg = { 0 };

	static DWORD previous_time = timeGetTime();
	static const float target_framerate = 60.0f;
	static const float max_time_step = 1.0f / target_framerate;

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			DWORD current_time = timeGetTime();
			float delta_time = (current_time - previous_time) / 1000.0f;
			previous_time = current_time;
			delta_time = std::min<float>(delta_time, max_time_step);

			update(delta_time);
			render();
		}
	}

	return static_cast<int>(msg.wParam);

}

HRESULT init_device()
{

	HRESULT hr = rhi.init(g_window_handle);

	//TODO: move gui initialization elsewhere
	if (!FAILED(hr))
	{
		ImGui_ImplDX11_Init(rhi.m_device.m_device, rhi.m_context);
	}

	return hr;
}

dx::XMMATRIX scaleMat = dx::XMMatrixScaling(1.0f, 1.0f, 1.0f);

HRESULT init_data()
{

	HRESULT hr;

	hr = rb.init();
	RETURN_IF_FAILED(hr);

	/*hr = shaders.Init();
	RETURN_IF_FAILED(hr);  */

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
	//Renderable_Entity mHnd18 = rf.load_model_from_file("./assets/brocc-the-athlete/source/Sporter_Retopo.fbx");
	Renderable_Entity mHnd19 = rf.load_model_from_file("./assets/Human2/head_and_jacket.gltf");
	rf.add_light(nullptr, dx::XMVectorSet(1.0f, 0.0f, 0.0, 0.0f), 1.0f, 8.0f);
	//rf.AddLight(nullptr, dx::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0.22f, 0.20f);

	//rhi.context->IASetInputLayout(shaders.vertexLayout);
	rhi.m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	////Camera setup-------------------------------------------------------------
	dx::XMVECTOR cam_pos = dx::XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f);
	dx::XMVECTOR cam_dir = dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	dx::XMVECTOR cam_up = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	g_cam = Camera(cam_pos, cam_dir, cam_up, dx::XM_PIDIV2 * rhi.m_viewport.Height / rhi.m_viewport.Width, rhi.m_viewport.Width / rhi.m_viewport.Height, 0.01f, 10.0f);
	//-----------------------------------------------------------------------------

	return hr;
}

float rot = 0.0f;

void render()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::ShowDemoWindow();

	rhi.m_context->ClearRenderTargetView(rb.m_final_render_target_view, dx::Colors::Black);
	rhi.m_context->ClearDepthStencilView(rb.m_depth_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	rhi.m_context->OMSetRenderTargets(1, &rb.m_final_render_target_view, rb.m_depth_dsv);
	rhi.m_context->RSSetViewports(1, &rb.m_scene_viewport);

	//g_cam.rotateAroundCamAxis(0.0f, -0.00f, 0.0f);    

	// Rendering cam view -----------------------------------------------------    
	const View_Desc view_desc = rf.compute_view(g_cam);
	rb.render_view(view_desc);
	//-------------------------------------------------------------

	rhi.m_context->ClearRenderTargetView(rhi.m_back_buffer_rtv, dx::Colors::Black);
	rhi.m_context->ClearDepthStencilView(rhi.m_back_buffer_depth_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	rhi.m_context->OMSetRenderTargets(1, &rhi.m_back_buffer_rtv, rhi.m_back_buffer_depth_dsv);
	rhi.m_context->RSSetViewports(1, &rhi.m_viewport);

	rb.m_annot->BeginEvent(L"Editor Pass");
	{
		//rhi.context->ResolveSubresource(rhi.editorSceneTex, 0, rtTexture, 0, DXGI_FORMAT_R8G8B8A8_UNORM);

		editor.render_editor(rf, g_cam, rb.m_final_render_target_srv);
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}
	rb.m_annot->EndEvent();

	//rhi.context->OMSetRenderTargets(1, &rhi.backBufferRTV, nullptr);
	//rhi.context->VSSetShader(shaders.vertexShaders.at((std::uint8_t)VShaderID::FULL_QUAD), nullptr, 0);
	//rhi.context->PSSetShader(shaders.pixelShaders.at((std::uint8_t)PShaderID::PRESENT), nullptr, 0);

	//rhi.context->PSSetShaderResources(0, 1, &rb.finalRenderTargetSRV);
	//rhi.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	//rhi.context->Draw(4, 0);
	//rhi.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//ID3D11ShaderResourceView* nullSRV[] = { nullptr };
	//rhi.context->PSSetShaderResources(0, 1, nullSRV);

	rhi.m_swap_chain->Present(g_enable_vsync, 0);
}

void update(float delta_time)
{
	rot += (0.3f * delta_time);
	if (rot > 6.28f)
	{
		rot = 0.0f;
	}
	rot = 0.0f;
}

void cleanup()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	buffer_cache.release_all_resources();
	resource_cache.release_all_resources();
	//shaders.ReleaseAllResources();
	rb.release_all_resources();
	rhi.release_all_resources();

	rhi.m_context->ClearState();
	rhi.m_context->Flush();

	if (rhi.m_context)rhi.m_context->Release();
	if (rhi.m_device.m_device) rhi.m_device.m_device->Release();

	if (g_debug_layer)
	{
		g_debug_layer->ReportLiveDeviceObjects(D3D11_RLDO_FLAGS::D3D11_RLDO_DETAIL);
		g_debug_layer->Release();
	}
}



int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(pCmdLine);


	if (!dx::XMVerifyCPUSupport())
	{
		MessageBox(NULL, "DirectXMath not supported by CPU", "Error", MB_OK);
		return -1;
	}

	if (init_application(hInstance, nCmdShow) != 0)
	{
		MessageBox(NULL, "Failed to create application window", "Error", MB_OK);
		return -1;
	}

	if (FAILED(init_device()))
	{
		MessageBox(NULL, "Failed to initialize correctly d3d device", "Error", MB_OK);
		cleanup();
		return -1;
	}

#ifdef _DEBUG
	if (FAILED(rhi.m_device.m_device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&g_debug_layer))))
	{
		MessageBox(NULL, "Failed to initialize correctly d3d debug layer", "Error", MB_OK);
		cleanup();
		return -1;
	}
#endif // _DEBUG

	if (FAILED(init_data()))
	{
		MessageBox(NULL, "Failed to initialize correctly scene data", "Error", MB_OK);
		cleanup();
		return -1;
	}

	int return_code = run();

	cleanup();


	//_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	//_CrtDumpMemoryLeaks();


	return return_code;
}
