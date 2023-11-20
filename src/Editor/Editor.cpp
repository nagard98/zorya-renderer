#include "Editor/Editor.h"
#include "Editor/SceneHierarchy.h"
#include "Editor/EntityOutline.h"

#include <vector>
#include <RendererFrontend.h>
#include <SceneGraph.h>

void Editor::RenderEditor(RendererFrontend& rf)
{
	std::uint32_t selectedID = -1;
	sceneHier.RenderSHierarchy(rf.sceneGraph, selectedID);
	if (selectedID != (std::uint32_t)-1)
	{
		Node<RenderableEntity>* selectedNode = rf.sceneGraph.findNode(rf.sceneGraph.rootNode, RenderableEntity{ selectedID });
		SubmeshInfo* smInfo = rf.findSubmeshInfo(selectedNode->value.submeshHnd);
		entityProps.RenderEProperties(selectedNode->value, smInfo, &rf.materials.at(selectedNode->value.submeshHnd.matDescIdx));
	}
}
