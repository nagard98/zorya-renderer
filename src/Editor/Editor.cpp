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
	strncpy(textBuff, "\0", 128);
}

void Editor::Init(const ImGuiID& dockspaceID) {
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
}

enum ClickableMenuItem {
	IMPORT = 0
};

void Editor::RenderEditor(RendererFrontend& rf, const ID3D11ShaderResourceView* rtSRV)
{
	bool clicked[] = {false};

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Import", "CTRL+I", &clicked[IMPORT])) {
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	if (clicked[IMPORT]) {
		ImGui::OpenPopup("import_model");
	}

	if (ImGui::BeginPopupModal("import_model", NULL)) {
		if (ImGui::InputText("model path", textBuff, 128)) {
		}
		if (ImGui::Button("Import")) {
			ImGui::CloseCurrentPopup();
			rf.LoadModelFromFile(std::string(textBuff));
		}
		ImGui::EndPopup();
	}

	ImGuiID dockspaceID	= ImGui::DockSpaceOverViewport();
	if (firstLoop) {
		Init(dockspaceID);
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
		float aspectRatio = 16.0f / 9.0f;
		float padding = 20.0f;
		ImVec2 size = ImGui::DockBuilderGetNode(sceneId)->Size;
		ImVec2 center = ImVec2(size.x / 2, size.y / 2);

		if (size.x / size.y > aspectRatio) {
			size.x = (size.y * 1280.0f) / 720.0f;
		}else{
			size.y = (size.x * 720.0f) / 1280.0f;
		}
		size.x -= (2 * padding);
		size.y -= (2 * padding);
		
		ImGui::SetCursorPos(ImVec2(center.x - (size.x / 2.0f), center.y - (size.y / 2.0f)));

		ImGui::Image((void*)rtSRV, size);
	}
	ImGui::End();

}
