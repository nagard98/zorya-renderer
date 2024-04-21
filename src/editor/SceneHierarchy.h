#ifndef SCENE_HIERARCHY_H_
#define SCENE_HIERARCHY_H_

#include "imgui.h"

#include <vector>

#include "renderer/frontend/RendererFrontend.h"
#include "renderer/frontend/SceneGraph.h"

namespace zorya
{
	class Scene_Hierarchy
	{

	public:
		Scene_Hierarchy();
		~Scene_Hierarchy();

		void render_scene_hierarchy(Renderer_Frontend& scene_manager, uint32_t& selected_entity);
		void render_scene_hierarchy_node(Node<Renderable_Entity>* entity);

		uint32_t m_selected_item;
		Node<Renderable_Entity>* m_node_to_remove;
		ImGuiTreeNodeFlags m_base_flags;

	};

}



#endif