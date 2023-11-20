#include "Editor/EntityOutline.h"
#include "RendererFrontend.h"
#include "Material.h"

#include "DirectXMath.h"
#include <cassert>

#include "imgui.h"


namespace dx = DirectX;

EntityOutline::EntityOutline()
{
}

EntityOutline::~EntityOutline()
{
}

void EntityOutline::RenderEProperties(RenderableEntity& entity, SubmeshInfo* smInfo, MaterialDesc* matDesc)
{
	float angles[3] = { dx::XM_2PI * 2.0f,0.0f,0.0f };

	ImGui::Begin("Entity Outline");

	ImGui::SeparatorText("Transform");
	{
		ImGui::DragFloat3("Position", &entity.localWorldTransf.pos.x, 0.01f);
		ImGui::DragFloat3("Rotation", &entity.localWorldTransf.rot.x, 0.01f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
		ImGui::DragFloat3("Scale", &entity.localWorldTransf.scal.x, 0.01f, 0.0f, 0.0f, "%.3f");
	}

	//TODO: better check if entity has mesh; probably move check to callee of this function
	if (smInfo != nullptr) {
		assert(matDesc != nullptr);
		ImGui::SeparatorText("Material");
		{
			ImGui::ColorEdit4("Base Color", &matDesc->baseColor.x);
			if (ImGui::IsItemEdited()) {
				smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
			}

			ImGui::SliderFloat("Smoothness", &matDesc->smoothness, 0, 1);
			if (ImGui::IsItemEdited()) {
				smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
			}

			ImGui::SliderFloat("Metalness", &matDesc->metalness, 0, 1);
			if (ImGui::IsItemEdited()) {
				smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
			}
			
			wcstombs(tmpCharBuff, matDesc->albedoPath, 128);
			ImGui::InputText("Albedo Map", tmpCharBuff, 128);
			if (ImGui::IsItemDeactivatedAfterEdit()) {
				mbstowcs(matDesc->albedoPath, tmpCharBuff, 128);
				smInfo->matCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
			}

			wcstombs(tmpCharBuff, matDesc->normalPath, 128);
			ImGui::InputText("Normal Map", tmpCharBuff, 128);
			if (ImGui::IsItemDeactivatedAfterEdit()) {
				mbstowcs(matDesc->normalPath, tmpCharBuff, 128);
				smInfo->matCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
			}

			wcstombs(tmpCharBuff, matDesc->metalnessMask, 128);
			ImGui::InputText("Metalness Mask", tmpCharBuff, 128);
			if (ImGui::IsItemDeactivatedAfterEdit()) {
				mbstowcs(matDesc->metalnessMask, tmpCharBuff, 128);
				smInfo->matCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
			}
		}
	}


	ImGui::End();

}