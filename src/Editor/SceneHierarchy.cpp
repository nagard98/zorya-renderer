#include "Editor/SceneHierarchy.h"

SceneHierarchy::SceneHierarchy()
{
	selectedItem = -1;
    baseFlags = ImGuiTreeNodeFlags_OpenOnArrow;
}

SceneHierarchy::~SceneHierarchy()
{
}

bool SceneHierarchy::Render(std::vector<RenderableEntity>& entities)
{
    ImGuiTreeNodeFlags nodeFlags;

    ImGui::Begin("Scene Hierarchy");

    for (RenderableEntity& entity : entities) {
        nodeFlags = baseFlags;
        //if (entity.modelHnd.numMeshes == 1) {
        //    nodeFlags |= ImGuiTreeNodeFlags_Leaf;
        //}
        if (entity.ID == selectedItem) {
            nodeFlags |= ImGuiTreeNodeFlags_Selected;
        }
        bool isNodeOpen = ImGui::TreeNodeEx(entity.entityName.c_str(), nodeFlags);
        if (ImGui::IsItemClicked()) {
            selectedItem = entity.ID;
        }
        if (isNodeOpen) {
            ImGui::Text(std::to_string(entity.modelHnd.baseMesh).c_str());
            ImGui::TreePop();
        }
    }

    ImGui::End();

	return false;
}


