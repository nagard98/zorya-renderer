#include "Editor/Editor.h"
#include "Editor/SceneHierarchy.h"
#include "Editor/EntityOutline.h"

#include <vector>
#include <RendererFrontend.h>
#include <SceneGraph.h>

#include "imgui.h"
#include "imgui_internal.h"
#include <d3d11_1.h>


Editor::Editor()
{
	firstLoop = true;
	sceneId = -1;
}

void Editor::RenderEditor(RendererFrontend& rf, const ID3D11ShaderResourceView* rtSRV)
{
	ImGui::BeginMainMenuBar();
	ImGui::MenuItem("File");
	ImGui::EndMainMenuBar();

	ImGuiID dockspaceID	= ImGui::DockSpaceOverViewport();
	if (firstLoop) {
		ImVec2 workSize = ImGui::GetMainViewport()->WorkSize;

		ImGui::DockBuilderRemoveNode(dockspaceID);
		ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceID, workSize);

		sceneId = dockspaceID;
		ImGuiID leftId = ImGui::DockBuilderSplitNode(sceneId, ImGuiDir_Left, 0.15f, nullptr, &sceneId);
		ImGuiID rightId = ImGui::DockBuilderSplitNode(sceneId, ImGuiDir_Right, 0.20f, nullptr, &sceneId);
		ImGuiID bottomId = ImGui::DockBuilderSplitNode(sceneId, ImGuiDir_Down, 0.30f, nullptr, &sceneId);

		ImGui::DockBuilderDockWindow("Hierarchy", leftId);
		ImGui::DockBuilderDockWindow("Entity Outline", rightId);
		ImGui::DockBuilderDockWindow("Asset Explorer", bottomId);
		ImGui::DockBuilderDockWindow("Scene", sceneId);

		ImGui::DockBuilderFinish(dockspaceID);

		firstLoop = false;
	}

	std::uint32_t selectedID = -1;

	ImGui::Begin("Hierarchy");
	{
		sceneHier.RenderSHierarchy(rf.sceneGraph, selectedID);
	}
	ImGui::End();

	ImGui::Begin("Entity Outline");
	{
		if (selectedID != (std::uint32_t)-1)
		{
			Node<RenderableEntity>* selectedNode = rf.sceneGraph.findNode(rf.sceneGraph.rootNode, RenderableEntity{ selectedID });
			SubmeshInfo* smInfo = rf.findSubmeshInfo(selectedNode->value.submeshHnd);
			entityProps.RenderEProperties(selectedNode->value, smInfo, &rf.materials.at(selectedNode->value.submeshHnd.matDescIdx));
		}
	}
	ImGui::End();

	ImGui::Begin("Asset Explorer");
	{}
	ImGui::End();

	ImGui::Begin("Scene");
	{
		ImVec2 size = ImGui::DockBuilderGetNode(sceneId)->Size;
		size.y = (size.x * 900.0f) / 1600.0f;
		ImGui::Image((void*)rtSRV, size);
	}
	ImGui::End();

}
