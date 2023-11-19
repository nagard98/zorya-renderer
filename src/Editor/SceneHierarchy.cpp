#include "Editor/SceneHierarchy.h"
#include "ResourceCache.h"
#include "imgui.h"

SceneHierarchy::SceneHierarchy()
{
	selectedItem = -1;
    baseFlags = ImGuiTreeNodeFlags_OpenOnArrow;
}

SceneHierarchy::~SceneHierarchy()
{
}

void SceneHierarchy::RenderSHierarchy(const std::vector<RenderableEntity>& entities, RenderableEntity* selectedEntity)
{
    ImGuiTreeNodeFlags nodeFlags;

    ImGui::Begin("Scene Hierarchy");

    for (const RenderableEntity& entity : entities) {
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
            //selectedEntity = entity.;
        }
        if (isNodeOpen) {
            ImGui::Text(std::to_string(entity.submeshHnd.baseIndex).c_str());
            ImGui::TreePop();
        }
    }

    ImGui::End();

}


