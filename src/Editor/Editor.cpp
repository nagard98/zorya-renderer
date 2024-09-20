#include "editor/Editor.h"
#include "editor/SceneHierarchy.h"
#include "editor/EntityOutline.h"
#include "editor/Logger.h"
#include "editor/AssetRegistry.h"

#include "renderer/frontend/SceneManager.h"
#include "renderer/frontend/SceneGraph.h"

#include <vector>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include <d3d11_1.h>
#include <Windows.h>

namespace zorya
{
	ImGui::FileBrowser file_browser;
	Editor editor;

	Editor::Editor()
	{
		m_first_loop = true;
		m_is_scene_clicked = false;
		m_scene_id = -1;
		strncpy_s(m_text_buffer, "\0", 128);
	}

	HRESULT Editor::init(HWND window_hnd, Render_Hardware_Interface& rhi)
	{
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		ImGui_ImplWin32_Init(window_hnd);
		ImGui_ImplDX11_Init(rhi.m_device.m_device, rhi.m_context);

		m_content_browser.init();

		//TODO: return meaningful value
		return S_OK;
	}

	void Editor::shutdown()
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	void Editor::setup_dockspace(const ImGuiID& dockspace_ID)
	{
		ImVec2 work_size = ImGui::GetMainViewport()->WorkSize;

		ImGui::DockBuilderRemoveNode(dockspace_ID);
		ImGui::DockBuilderAddNode(dockspace_ID, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace_ID, work_size);

		m_scene_id = dockspace_ID;
		ImGuiID right_id = ImGui::DockBuilderSplitNode(m_scene_id, ImGuiDir_Right, 0.20f, nullptr, &m_scene_id);
		ImGuiID bottom_id = ImGui::DockBuilderSplitNode(m_scene_id, ImGuiDir_Down, 0.3f, nullptr, &m_scene_id);
		ImGuiID left_id = ImGui::DockBuilderSplitNode(m_scene_id, ImGuiDir_Left, 0.15f, nullptr, &m_scene_id);


		ImGui::DockBuilderDockWindow("Hierarchy", left_id);
		ImGui::DockBuilderDockWindow("Entity Outline", right_id);
		ImGui::DockBuilderDockWindow("Asset Inspector", right_id);
		ImGui::DockBuilderDockWindow("Asset Explorer", bottom_id);
		ImGui::DockBuilderDockWindow("Logger", bottom_id);
		ImGui::DockBuilderDockWindow("Scene", m_scene_id);

		ImGui::DockBuilderFinish(dockspace_ID);
	}

	enum ClickableMenuItem
	{
		IMPORT = 0,
		ADD_POINT_LIGHT = 1,
		ADD_SPOT_LIGHT = 2,
		ADD_DIR_LIGHT = 3,
		ADD_SPHERE = 4
	};

	void Editor::new_frame()
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}

	void Editor::render_editor(Scene_Manager& frontend_renderer, Camera& editor_cam, const ID3D11ShaderResourceView* scene_view_srv)
	{
		ImGui::ShowDemoWindow();

		bool clicked[5] = { false };

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Import", "CTRL+I", &clicked[IMPORT]))
				{
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Scene"))
			{
				if (ImGui::BeginMenu("Add"))
				{
					if (ImGui::BeginMenu("Light"))
					{
						if (ImGui::MenuItem("Directional", nullptr, &clicked[ADD_DIR_LIGHT])) {}
						if (ImGui::MenuItem("Point", nullptr, &clicked[ADD_POINT_LIGHT])) {}
						if (ImGui::MenuItem("Spot", nullptr, &clicked[ADD_SPOT_LIGHT])) {}
						ImGui::EndMenu();
					}
					/*if (ImGui::MenuItem("Sphere", nullptr, &clicked[ADD_SPHERE])) {}*/
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		if (clicked[IMPORT])
		{
			m_file_browser_open = true;
			file_browser.Open();
		}

		if (clicked[ADD_DIR_LIGHT])
		{
			frontend_renderer.add_light(frontend_renderer.m_scene_graph.root_node, dx::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f));
		}

		if (clicked[ADD_SPOT_LIGHT])
		{
			frontend_renderer.add_light(frontend_renderer.m_scene_graph.root_node, dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), dx::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), dx::XM_PIDIV4);
		}

		if (clicked[ADD_POINT_LIGHT])
		{
			frontend_renderer.add_light(frontend_renderer.m_scene_graph.root_node, dx::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0.22f, 0.20f);
		}

		file_browser.Display();

		if (file_browser.HasSelected())
		{
			const auto& file = file_browser.GetSelected();
			const char* dest_folder_path = asset_registry.current_node->value.m_folder_path;

			size_t new_filepath_buff_size = strlen(dest_folder_path) + 1 + file.filename().u8string().length() + 1;
			char* new_filepath = (char*)malloc(new_filepath_buff_size);
			ZeroMemory(new_filepath, new_filepath_buff_size);
			
			strcat_s(new_filepath, new_filepath_buff_size, dest_folder_path);
			strcat_s(new_filepath, new_filepath_buff_size, "\\");
			strcat_s(new_filepath, new_filepath_buff_size, file.filename().u8string().c_str());

			int copy_success = CopyFile(file.u8string().c_str(), new_filepath, true);
			if (copy_success != 0 && (file.extension().u8string()).compare(".gltf") == 0)
			{
				auto bin_filepath = file;
				bin_filepath.replace_extension(std::filesystem::path(".bin"));

				char new_bin_filepath[MAX_PATH];
				ZeroMemory(new_bin_filepath, MAX_PATH);

				sprintf(new_bin_filepath, "%s\\%s", dest_folder_path, bin_filepath.filename().u8string().c_str());
				copy_success = CopyFile(bin_filepath.u8string().c_str(), new_bin_filepath, true);
			}

			if(copy_success != 0)
			{
				Asset_Import_Config* asset_imp_conf = create_asset_import_config(get_asset_type_by_ext(file.extension().u8string().c_str()), new_filepath);
				asset_registry.current_node->children.emplace_back(Filesystem_Node(Filesystem_Node_Type::FILE, nullptr, asset_imp_conf, file.filename().u8string().c_str()));
			} 
			else
			{
				//TODO: when copy fails give a message
			}

			file_browser.ClearSelected();
		}


		ImGuiID dockspace_id = ImGui::DockSpaceOverViewport();
		if (m_first_loop)
		{
			setup_dockspace(dockspace_id);
			m_first_loop = false;
		}

		uint32_t selected_id = -1;

		ImGui::Begin("Hierarchy");
		{
			auto start_cursor_pos = ImGui::GetCursorPos();
			m_scene_hierarchy.render_scene_hierarchy(frontend_renderer, selected_id);
			
			ImGui::SetCursorPos(start_cursor_pos);
			ImGui::Dummy(ImGui::GetWindowSize());
			if (ImGui::BeginDragDropTarget())
			{
				if (auto payload = ImGui::AcceptDragDropPayload(Asset_Type_Names[Asset_Type::STATIC_MESH]))
				{
					Asset_With_Config* dropped_payload = static_cast<Asset_With_Config*>(payload->Data);
					Model3D* model = static_cast<Model3D*>(dropped_payload->asset);
					build_renderable_entity_tree(scene_manager.m_scene_graph.root_node, model, 0);
				}
				ImGui::EndDragDropTarget();
			}
		}
		ImGui::End();

		ImGui::Begin("Entity Outline");
		{
			if (selected_id != (uint32_t)-1)
			{
				Node<Renderable_Entity>* parent_node = nullptr;
				Node<Renderable_Entity>* selected_node = frontend_renderer.m_scene_graph.find_node(frontend_renderer.m_scene_graph.root_node, Renderable_Entity{ selected_id }, &parent_node);
				//TODO: change to something more generic, instead of submesh info; maybe here is where i should use reflection
				switch (selected_node->value.tag)
				{
				case Entity_Type::MESH:
				{
					m_entity_properties.render_entity_mesh_properties(selected_node->value);
					break;
				}
				case Entity_Type::COLLECTION:
				{
					m_entity_properties.render_entity_transform(selected_node->value);
					break;
				}
				case Entity_Type::LIGHT:
				{
					Light_Info& light_info = frontend_renderer.m_scene_lights.at(selected_node->value.hnd_light.index);
					m_entity_properties.render_entity_light_properties(selected_node->value, light_info);
					break;
				}
				default:
				{
					//TODO: implement other cases OR implement reflection so to not use this switching
					assert(false);
				}

				}

			}
		}
		ImGui::End();

		ImGui::Begin("Asset Inspector");
		{
			m_asset_inspector.render(scene_manager, m_content_browser.selected_asset);
		}
		ImGui::End();

		ImGui::Begin("Asset Explorer");
		{
			m_content_browser.render(asset_registry);
		}
		ImGui::End();

		ImGui::Begin("Logger");
		{
			Logger::render_logs();
		}
		ImGui::End();

		ImGui::Begin("Scene");
		{
			float aspect_ratio = 16.0f / 9.0f;
			float padding = 20.0f;
			ImVec2 size = ImGui::DockBuilderGetNode(m_scene_id)->Size;
			ImVec2 center = ImVec2(size.x / 2, size.y / 2);

			if (size.x / size.y > aspect_ratio)
			{
				size.x = (size.y * 1280.0f) / 720.0f;
			}
			else
			{
				size.y = (size.x * 720.0f) / 1280.0f;
			}
			size.x -= (2 * padding);
			size.y -= (2 * padding);

			ImGui::SetCursorPos(ImVec2(center.x - (size.x / 2.0f), center.y - (size.y / 2.0f)));

			ImGui::Image((void*)scene_view_srv, size);
			bool is_scene_hovered = ImGui::IsItemHovered();
			if (!m_is_scene_clicked && is_scene_hovered)
			{
				m_is_scene_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right);
			}

			if (m_is_scene_clicked)
			{
				ImVec2 delta = ImVec2(0.0, 0.0);
				if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
				{
					delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
					editor_cam.rotate_around_focus_point(delta.y / 100.0f, delta.x / 100.0f);
					ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
				}

				if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
				{
					delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
					editor_cam.translate_along_cam_axis(-delta.x / 100.0f, delta.y / 100.0f, 0.0f);
					ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
				}
			}

			if (is_scene_hovered)
			{
				if (ImGui::IsMouseKey(ImGuiKey_MouseWheelY))
				{
					ImGuiIO& io = ImGui::GetIO();
					float zoom = io.MouseWheel / 5.0f;
					editor_cam.translate_along_cam_axis(0.0f, 0.0f, zoom);
				}
			}

			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Right))
			{
				m_is_scene_clicked = false;
			}
		}
		ImGui::End();

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	}

}