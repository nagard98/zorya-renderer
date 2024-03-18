#include "editor/Editor.h"
#include "editor/SceneHierarchy.h"
#include "editor/EntityOutline.h"
#include "editor/Logger.h"

#include "renderer/frontend/RendererFrontend.h"
#include "renderer/frontend/SceneGraph.h"

#include <vector>

#include "imgui.h"
#include "imgui_internal.h"

#include <d3d11_1.h>


Editor::Editor()
{
	firstLoop = true;
	isSceneClicked = false;
	sceneId = -1;
	strncpy_s(textBuff, "\0", 128);
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
	ImGui::DockBuilderDockWindow("Logger", bottomId);
	ImGui::DockBuilderDockWindow("Scene", sceneId);

	ImGui::DockBuilderFinish(dockspaceID);
}

enum ClickableMenuItem {
	IMPORT = 0,
	ADD_POINT_LIGHT = 1,
	ADD_SPOT_LIGHT = 2,
	ADD_DIR_LIGHT = 3,
	ADD_SPHERE = 4
};

void Editor::RenderEditor(RendererFrontend& rf, Camera& editorCam, const ID3D11ShaderResourceView* rtSRV)
{
	bool clicked[5] = {false};

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Import", "CTRL+I", &clicked[IMPORT])) {
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Scene")) {
			if (ImGui::BeginMenu("Add")) {
				if (ImGui::BeginMenu("Light")) {
					if (ImGui::MenuItem("Directional", nullptr, &clicked[ADD_DIR_LIGHT])) {}
					if (ImGui::MenuItem("Point", nullptr, &clicked[ADD_POINT_LIGHT])) {}
					if (ImGui::MenuItem("Spot", nullptr, &clicked[ADD_SPOT_LIGHT])) {}
					ImGui::EndMenu();
				}
				/*if (ImGui::MenuItem("Sphere", nullptr, &clicked[ADD_SPHERE])) {}*/
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	if (clicked[IMPORT]) {
		ImGui::OpenPopup("import_model");
	}

	if (clicked[ADD_DIR_LIGHT]) {
		rf.AddLight(nullptr, dx::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f));
	}

	if (clicked[ADD_SPOT_LIGHT]) {
		rf.AddLight(nullptr, dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), dx::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), dx::XM_PIDIV4);
	}

	if (clicked[ADD_POINT_LIGHT]) {
		rf.AddLight(nullptr, dx::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0.22f, 0.20f);
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
		sceneHier.RenderSHierarchy(rf, selectedID);
	}
	ImGui::End();

	ImGui::Begin("Entity Outline");
	{
		if (selectedID != (std::uint32_t)-1)
		{
			Node<RenderableEntity>* parentNode = nullptr;
			Node<RenderableEntity>* selectedNode = rf.sceneGraph.findNode(rf.sceneGraph.rootNode, RenderableEntity{ selectedID }, &parentNode);
			//TODO: change to something more generic, instead of submesh info; maybe here is where i should use reflection
			switch (selectedNode->value.tag)
			{
				case EntityType::MESH:
				{
					SubmeshInfo* smInfo = rf.findSubmeshInfo(selectedNode->value.submeshHnd);
					entityProps.RenderEProperties(selectedNode->value, smInfo, &rf.materials.at(selectedNode->value.submeshHnd.matDescIdx));
					break;
				}
				case EntityType::COLLECTION:
				{
					SubmeshInfo* smInfo = rf.findSubmeshInfo(selectedNode->value.submeshHnd);
					entityProps.RenderEProperties(selectedNode->value, smInfo, &rf.materials.at(selectedNode->value.submeshHnd.matDescIdx));
					break;
				}
				case EntityType::LIGHT:
				{
					LightInfo& lightInfo = rf.sceneLights.at(selectedNode->value.lightHnd.index);
					entityProps.RenderEProperties(selectedNode->value, lightInfo);
					break;
				}
				default:
				{
					//TODO: implement other cases OR implement reflection so to not use this switching
					assert(false);
				}

			}

		}
	}
	ImGui::End();

	ImGui::Begin("Logger");
	{
		Logger::RenderLogs();
	}
	ImGui::End();

	ImGui::Begin("Asset Explorer");
	{

	}
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
		bool isSceneHovered = ImGui::IsItemHovered();
		if (!isSceneClicked && isSceneHovered) {
			isSceneClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right);
		}

		if (isSceneClicked) {
			ImVec2 delta = ImVec2(0.0, 0.0);
			if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
				delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
				editorCam.rotateAroundCamAxis(delta.y / 100.0f, delta.x / 100.0f, 0.0f);
				ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
			}

			if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
				delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
				editorCam.translateAlongCamAxis(-delta.x / 100.0f, delta.y / 100.0f, 0.0f);
				ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
			}
		}

		if (isSceneHovered) {
			if (ImGui::IsMouseKey(ImGuiKey_MouseWheelY)) {
				ImGuiIO& io = ImGui::GetIO();
				float zoom = io.MouseWheel / 5.0f;
				editorCam.translateAlongCamAxis(0.0f, 0.0f, zoom);
			}
		}

		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
			isSceneClicked = false;
		}
	}
	ImGui::End();



}
