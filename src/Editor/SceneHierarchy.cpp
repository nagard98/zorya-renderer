#include "editor/SceneHierarchy.h"

#include "renderer/backend/ResourceCache.h"

#include "renderer/frontend/SceneGraph.h"
#include "renderer/frontend/RendererFrontend.h"

#include "imgui.h"

#include <vector>

namespace zorya
{
	Scene_Hierarchy::Scene_Hierarchy()
	{
		m_selected_item = -1;
		m_node_to_remove = nullptr;
		m_base_flags = ImGuiTreeNodeFlags_OpenOnArrow;
	}

	Scene_Hierarchy::~Scene_Hierarchy()
	{}

	void Scene_Hierarchy::render_scene_hierarchy(Renderer_Frontend& scene_manager, uint32_t& selected_entity)
	{
		Scene_Graph<Renderable_Entity>& entities = scene_manager.m_scene_graph;

		for (Node<Renderable_Entity>* entity : entities.root_node->children)
		{
			render_scene_hierarchy_node(entity);
		}

		if (m_node_to_remove != nullptr)
		{
			scene_manager.remove_entity(m_node_to_remove->value);
			m_node_to_remove = nullptr;
			m_selected_item = -1;
		}

		selected_entity = m_selected_item;

	}

	void Scene_Hierarchy::render_scene_hierarchy_node(Node<Renderable_Entity>* entity)
	{
		ImGuiTreeNodeFlags node_flags = m_base_flags;
		if (entity->children.size() == 0)
		{
			node_flags |= ImGuiTreeNodeFlags_Leaf;
		}
		if (entity->value.ID == m_selected_item)
		{
			node_flags |= ImGuiTreeNodeFlags_Selected;
		}

		ImGui::PushID(entity->value.ID);
		bool is_node_open = ImGui::TreeNodeEx(entity->value.entity_name.c_str(), node_flags);
		if (ImGui::IsItemClicked())
		{
			m_selected_item = entity->value.ID;
		}

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::Selectable("Remove"))
			{
				m_node_to_remove = entity;
			}
			ImGui::EndPopup();
		}

		ImGui::PopID();

		if (is_node_open)
		{
			for (Node<Renderable_Entity>* child_entity : entity->children)
			{
				render_scene_hierarchy_node(child_entity);
			}
			ImGui::TreePop();
		}
	}
}


