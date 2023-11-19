#include "Editor/Editor.h"

#include <vector>
#include <RendererFrontend.h>

Editor::Editor()
{
}

Editor::~Editor()
{
}

void Editor::RenderEditor(const std::vector<RenderableEntity>& entities)
{
	sceneHier.RenderSHierarchy(entities, nullptr);
	entityProps.RenderEProperties();
}
