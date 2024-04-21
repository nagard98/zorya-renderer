#include "editor/Editor.h"
#include "editor/SceneHierarchy.h"
#include "editor/EntityOutline.h"
#include "editor/Logger.h"

#include "renderer/frontend/RendererFrontend.h"
#include "renderer/frontend/SceneGraph.h"

#include <vector>

#include "imgui.h"
#include "imgui_internal.h"

#include <d3d11_1.h>

namespace zorya
{
	Editor::Editor()
	{
		m_first_loop = true;
		m_is_scene_clicked = false;
		m_scene_id = -1;
		strncpy_s(m_text_buffer, "\0", 128);
	}

	void Editor::init(const ImGuiID& dockspace_ID)
	{
		ImVec2 work_size = ImGui::GetMainViewport()->WorkSize;

		ImGui::DockBuilderRemoveNode(dockspace_ID);
		ImGui::DockBuilderAddNode(dockspace_ID, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace_ID, work_size);

		m_scene_id = dockspace_ID;
		ImGuiID left_id = ImGui::DockBuilderSplitNode(m_scene_id, ImGuiDir_Left, 0.15f, nullptr, &m_scene_id);
		ImGuiID right_id = ImGui::DockBuilderSplitNode(m_scene_id, ImGuiDir_Right, 0.20f, nullptr, &m_scene_id);
		ImGuiID bottom_id = ImGui::DockBuilderSplitNode(m_scene_id, ImGuiDir_Down, 0.30f, nullptr, &m_scene_id);

		ImGui::DockBuilderDockWindow("Hierarchy", left_id);
		ImGui::DockBuilderDockWindow("Entity Outline", right_id);
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

	void Editor::render_editor(Renderer_Frontend& frontend_renderer, Camera& editor_cam, const ID3D11ShaderResourceView* scene_view_srv)
	{
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

			//ImGui::OpenPopup("import_model");
		}

		if (clicked[ADD_DIR_LIGHT])
		{
			frontend_renderer.add_light(nullptr, dx::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f));
		}

		if (clicked[ADD_SPOT_LIGHT])
		{
			frontend_renderer.add_light(nullptr, dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), dx::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), dx::XM_PIDIV4);
		}

		if (clicked[ADD_POINT_LIGHT])
		{
			frontend_renderer.add_light(nullptr, dx::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0.22f, 0.20f);
		}

		if (ImGui::BeginPopupModal("import_model", NULL))
		{
			if (ImGui::InputText("model path", m_text_buffer, 128))
			{
			}
			if (ImGui::Button("Import"))
			{
				ImGui::CloseCurrentPopup();
				frontend_renderer.load_model_from_file(std::string(m_text_buffer));
			}
			ImGui::EndPopup();
		}

		file_browser.Display();

		if (file_browser.HasSelected())
		{
			frontend_renderer.load_model_from_file(file_browser.GetSelected().u8string());
			file_browser.ClearSelected();
		}


		ImGuiID dockspace_id = ImGui::DockSpaceOverViewport();
		if (m_first_loop)
		{
			init(dockspace_id);
			m_first_loop = false;
		}

		uint32_t selected_id = -1;

		ImGui::Begin("Hierarchy");
		{
			m_scene_hierarchy.render_scene_hierarchy(frontend_renderer, selected_id);
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
					Submesh_Info* submesh_info = frontend_renderer.find_submesh_info(selected_node->value.hnd_submesh);
					m_entity_properties.render_entity_properties(selected_node->value, submesh_info, frontend_renderer.m_materials.at(selected_node->value.hnd_submesh.material_desc_id));
					break;
				}
				case Entity_Type::COLLECTION:
				{
					Submesh_Info* submesh_info = frontend_renderer.find_submesh_info(selected_node->value.hnd_submesh);
					m_entity_properties.render_entity_properties(selected_node->value, submesh_info, frontend_renderer.m_materials.at(selected_node->value.hnd_submesh.material_desc_id));
					break;
				}
				case Entity_Type::LIGHT:
				{
					Light_Info& light_info = frontend_renderer.m_scene_lights.at(selected_node->value.hnd_light.index);
					m_entity_properties.render_entity_properties(selected_node->value, light_info);
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

		ImGui::Begin("Logger");
		{
			Logger::render_logs();
		}
		ImGui::End();

		ImGui::Begin("Asset Explorer");
		{

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

	}

}