#include "editor/SceneHierarchy.h"

#include "renderer/backend/ResourceCache.h"

#include "renderer/frontend/SceneGraph.h"
#include "renderer/frontend/SceneManager.h"

#include "imgui.h"

#include <vector>

namespace zorya
{
	Scene_Hierarchy::Scene_Hierarchy()
	{
		m_selected_item = -1;
		m_node_to_remove = nullptr;
		m_parent_of_node_to_remove = nullptr;
		m_base_flags = ImGuiTreeNodeFlags_OpenOnArrow;
	}

	Scene_Hierarchy::~Scene_Hierarchy()
	{}

	void Scene_Hierarchy::render_scene_hierarchy(Scene_Manager& scene_manager, uint32_t& selected_entity)
	{
		Scene_Graph<Renderable_Entity>& entities = scene_manager.m_scene_graph;

		for (Node<Renderable_Entity>* entity : entities.root_node->children)
		{
			render_scene_hierarchy_node(entity, entities.root_node);
		}

		if (m_node_to_remove != nullptr)
		{
			scene_manager.remove_node(m_node_to_remove, m_parent_of_node_to_remove); //entity(m_node_to_remove->value);
			m_node_to_remove = nullptr;
			m_selected_item = -1;
		}

		selected_entity = m_selected_item;

	}

	void Scene_Hierarchy::render_scene_hierarchy_node(Node<Renderable_Entity>* entity, Node<Renderable_Entity>* parent_node)
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
			ImGui::SetWindowFocus("Entity Outline");
		}
		if (ImGui::BeginDragDropTarget())
		{
			if (auto payload = ImGui::AcceptDragDropPayload(Asset_Type_Names[Asset_Type::STATIC_MESH]))
			{
				Asset_With_Config* dropped_payload = static_cast<Asset_With_Config*>(payload->Data);
				Model3D* model = static_cast<Model3D*>(dropped_payload->asset);
				build_renderable_entity_tree(entity, model, 0);
			}
			ImGui::EndDragDropTarget();
		}

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::Selectable("Remove"))
			{
				m_node_to_remove = entity;
				m_parent_of_node_to_remove = parent_node;
			}
			ImGui::EndPopup();
		}

		ImGui::PopID();

		if (is_node_open)
		{
			for (Node<Renderable_Entity>* child_entity : entity->children)
			{
				render_scene_hierarchy_node(child_entity, entity);
			}
			ImGui::TreePop();
		}
	}
}


