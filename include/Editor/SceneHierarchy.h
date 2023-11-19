#include <imgui.h>

#include <vector>
#include <RendererFrontend.h>

class SceneHierarchy {

public:
	SceneHierarchy();
	~SceneHierarchy();

	void RenderSHierarchy(const std::vector<RenderableEntity> &entities, RenderableEntity* selectedEntity);

	std::uint32_t selectedItem;
	ImGuiTreeNodeFlags baseFlags;

};