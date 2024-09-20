#define _CRTDBG_MAP_ALLOC

#include <crtdbg.h>
#include <Windows.h>
#include <hidusage.h>
#include <wrl/client.h>

#include <d3d11_1.h>
#include <DirectXMath.h>
#include <DirectXColors.h>

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include "Platform.h"
#include "ApplicationConfig.h"

#include "editor/Logger.h"
#include "editor/Editor.h"
#include "editor/AssetRegistry.h"

#include "renderer/frontend/Camera.h"
#include "renderer/frontend/SceneManager.h"
#include "renderer/frontend/PipelineStateManager.h"

#include "renderer/backend/rhi/RHIState.h"
#include "renderer/backend/rhi/RenderHardwareInterface.h"
#include "renderer/backend/Renderer.h"

#include "xxhash.h"

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")
#pragma comment (lib, "winmm.lib")


namespace dx = DirectX;
namespace wrl = Microsoft::WRL;

HWND g_window_handle = 0;

Camera g_cam;
float rot = 0.0f;

struct Mouse
{
	std::int64_t rel_x, rel_y;
};

Mouse mouse;

dx::XMMATRIX scaleMat = dx::XMMatrixScaling(1.0f, 1.0f, 1.0f);

//-----------------------------------------------------

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void update(float delta_time);
void render();

HRESULT startup_subsystems();
void shutdown_subsystems();

HRESULT init_data();


int init_application(HINSTANCE hInstance, int cmd_show)
{
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
	AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW | WS_MAXIMIZE, FALSE);

	g_window_handle = CreateWindow(
		g_windowClassName,
		g_windowName,
		WS_OVERLAPPEDWINDOW | WS_MAXIMIZE,
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


HRESULT init_data()
{
	HRESULT hr = S_OK;

	scene_manager.add_light(scene_manager.m_scene_graph.root_node, dx::XMVectorSet(1.0f, 0.0f, 0.0, 0.0f), 1.0f, 8.0f);
	//scene_manager.add_light(scene_manager.m_scene_graph.root_node, dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), dx::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), dx::XM_PIDIV4);

	////Camera setup-------------------------------------------------------------
	dx::XMVECTOR cam_pos = dx::XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f);
	dx::XMVECTOR cam_dir = dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	dx::XMVECTOR cam_up = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	g_cam = Camera(cam_pos, cam_dir, cam_up, dx::XM_PIDIV2 * (rhi.m_viewport.Height / rhi.m_viewport.Width), rhi.m_viewport.Width / rhi.m_viewport.Height, 0.01f, 100.0f);
	//-----------------------------------------------------------------------------

	return hr;
}


void render()
{
	editor.new_frame();

	// Rendering cam view -----------------------------------------------------    
	const View_Desc view_desc = scene_manager.compute_view(g_cam);
	renderer.render_view(view_desc);
	//-------------------------------------------------------------

	//renderer.m_annot->BeginEvent(L"Editor Pass");
	//{
		rhi.m_context->ClearRenderTargetView(rhi.m_back_buffer_rtv, dx::Colors::Black);
		rhi.m_context->ClearDepthStencilView(rhi.m_back_buffer_depth_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		rhi.m_context->OMSetRenderTargets(1, &rhi.m_back_buffer_rtv, rhi.m_back_buffer_depth_dsv);
		rhi.m_context->RSSetViewports(1, &rhi.m_viewport);

		editor.render_editor(scene_manager, g_cam, rhi.get_srv_pointer(renderer.hnd_final_srv));
	//}
	//renderer.m_annot->EndEvent();

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

HRESULT startup_subsystems()
{
	HRESULT hr = S_OK;

	RETURN_IF_FAILED2(hr, rhi.init(g_window_handle));
	RETURN_IF_FAILED2(hr, scene_manager.init());
	RETURN_IF_FAILED2(hr, renderer.init());
	RETURN_IF_FAILED2(hr, asset_registry.init(".\\assets\\brocc-the-athlete"));
	RETURN_IF_FAILED2(hr, editor.init(g_window_handle, rhi));
	RETURN_IF_FAILED2(hr, pipeline_state_manager.init());

	return hr;
}

void shutdown_subsystems()
{
	pipeline_state_manager.shutdown();
	editor.shutdown();

	buffer_cache.release_all_resources();
	resource_cache.release_all_resources();

	renderer.shutdown();

	if (rhi.m_debug_layer)
	{
		rhi.m_debug_layer->ReportLiveDeviceObjects(D3D11_RLDO_FLAGS::D3D11_RLDO_DETAIL);
	}

	rhi.shutdown();
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

	if (FAILED(startup_subsystems()))
	{
		MessageBox(NULL, "Failed to startup correctly engine subsystems", "Error", MB_OK);
		shutdown_subsystems();
		return -1;
	}

#ifdef _DEBUG
	if (FAILED(rhi.m_device.m_device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&rhi.m_debug_layer))))
	{
		MessageBox(NULL, "Failed to initialize correctly d3d debug layer", "Error", MB_OK);
		shutdown_subsystems();
		return -1;
	}
#endif // _DEBUG

	if (FAILED(init_data()))
	{
		MessageBox(NULL, "Failed to initialize correctly scene data", "Error", MB_OK);
		shutdown_subsystems();
		return -1;
	}


	int return_code = run();

	shutdown_subsystems();

	//_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	//_CrtDumpMemoryLeaks();

	return return_code;
}
