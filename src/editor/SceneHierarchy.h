#ifndef SCENE_HIERARCHY_H_
#define SCENE_HIERARCHY_H_

#include "imgui.h"

#include <vector>

#include "renderer/frontend/RendererFrontend.h"
#include "renderer/frontend/SceneGraph.h"

class SceneHierarchy {

public:
	SceneHierarchy();
	~SceneHierarchy();

	void RenderSHierarchy(RendererFrontend& sceneManager, std::uint32_t& selectedEntity);
	void RenderSHNode(Node<RenderableEntity>* entity);

	std::uint32_t selectedItem;
	Node<RenderableEntity>* nodeToRemove;
	ImGuiTreeNodeFlags baseFlags;

};

#endif