#include <imgui.h>

#include <vector>
#include <RendererFrontend.h>

class SceneHierarchy {

public:
	SceneHierarchy();
	~SceneHierarchy();

	bool Render(std::vector<RenderableEntity> &entities);

	std::uint32_t selectedItem;
	ImGuiTreeNodeFlags baseFlags;

};