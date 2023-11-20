#include <Editor/SceneHierarchy.h>
#include "ResourceCache.h"
#include "imgui.h"
#include "SceneGraph.h"
#include "RendererFrontend.h"

SceneHierarchy::SceneHierarchy()
{
	selectedItem = -1;
    baseFlags = ImGuiTreeNodeFlags_OpenOnArrow;
}

SceneHierarchy::~SceneHierarchy()
{
}

void SceneHierarchy::RenderSHierarchy(const SceneGraph<RenderableEntity>& entities, std::uint32_t& selectedEntity)
{

    for (const Node<RenderableEntity>* entity : entities.rootNode->children) {
        RenderSHNode(entity);
    }

    selectedEntity = selectedItem;

}

void SceneHierarchy::RenderSHNode(const Node<RenderableEntity>* entity)
{
    ImGuiTreeNodeFlags nodeFlags = baseFlags;
    if (entity->children.size() == 0) {
        nodeFlags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (entity->value.ID == selectedItem) {
        nodeFlags |= ImGuiTreeNodeFlags_Selected;
    }
    bool isNodeOpen = ImGui::TreeNodeEx(entity->value.entityName.c_str(), nodeFlags);
    if (ImGui::IsItemClicked()) {
        selectedItem = entity->value.ID;
    }
    if (isNodeOpen) {
        for (const Node<RenderableEntity>* childEnt : entity->children) {
            RenderSHNode(childEnt);
        }
        ImGui::TreePop();
    }
}


