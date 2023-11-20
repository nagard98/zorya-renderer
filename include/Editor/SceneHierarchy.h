#ifndef SCENE_HIERARCHY_H_
#define SCENE_HIERARCHY_H_

#include <imgui.h>

#include <vector>
#include <RendererFrontend.h>
#include <SceneGraph.h>

class SceneHierarchy {

public:
	SceneHierarchy();
	~SceneHierarchy();

	void RenderSHierarchy(const SceneGraph<RenderableEntity> &entities, std::uint32_t& selectedEntity);
	void RenderSHNode(const Node<RenderableEntity>* entity);

	std::uint32_t selectedItem;
	ImGuiTreeNodeFlags baseFlags;

};

#endif