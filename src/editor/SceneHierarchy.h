#ifndef SCENE_HIERARCHY_H_
#define SCENE_HIERARCHY_H_

#include "imgui.h"

#include <vector>

#include "renderer/frontend/SceneManager.h"
#include "renderer/frontend/SceneGraph.h"

namespace zorya
{
	class Scene_Hierarchy
	{

	public:
		Scene_Hierarchy();
		~Scene_Hierarchy();

		void render_scene_hierarchy(Scene_Manager& scene_manager, uint32_t& selected_entity);
		void render_scene_hierarchy_node(Node<Renderable_Entity>* entity, Node<Renderable_Entity>* parent_node);

		uint32_t m_selected_item;
		Node<Renderable_Entity>* m_node_to_remove;
		Node<Renderable_Entity>* m_parent_of_node_to_remove;
		ImGuiTreeNodeFlags m_base_flags;

	};

}



#endif