#include <Editor/SceneHierarchy.h>
#include "ResourceCache.h"
#include "imgui.h"
#include "SceneGraph.h"
#include "RendererFrontend.h"

#include <vector>

SceneHierarchy::SceneHierarchy()
{
	selectedItem = -1;
    nodeToRemove = nullptr;
    baseFlags = ImGuiTreeNodeFlags_OpenOnArrow;
}

SceneHierarchy::~SceneHierarchy()
{
}

void SceneHierarchy::RenderSHierarchy(RendererFrontend& sceneManager, std::uint32_t& selectedEntity)
{
    SceneGraph<RenderableEntity>& entities = sceneManager.sceneGraph;

    for (Node<RenderableEntity>* entity : entities.rootNode->children) {
        RenderSHNode(entity);
    }

    if (nodeToRemove != nullptr) {
        sceneManager.removeEntity(nodeToRemove->value);
        nodeToRemove = nullptr;
        selectedItem = -1;
    }

    selectedEntity = selectedItem;

}

void SceneHierarchy::RenderSHNode(Node<RenderableEntity>* entity)
{
    ImGuiTreeNodeFlags nodeFlags = baseFlags;
    if (entity->children.size() == 0) {
        nodeFlags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (entity->value.ID == selectedItem) {
        nodeFlags |= ImGuiTreeNodeFlags_Selected;
    }

    ImGui::PushID(entity->value.ID);
    bool isNodeOpen = ImGui::TreeNodeEx(entity->value.entityName.c_str(), nodeFlags);
    if (ImGui::IsItemClicked()) {
        selectedItem = entity->value.ID;
    }

    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::Selectable("Remove")) {
            nodeToRemove = entity;
        }
        ImGui::EndPopup();
    }

    ImGui::PopID();

    if (isNodeOpen) {
        for (Node<RenderableEntity>* childEnt : entity->children) {
            RenderSHNode(childEnt);
        }
        ImGui::TreePop();
    }
}


