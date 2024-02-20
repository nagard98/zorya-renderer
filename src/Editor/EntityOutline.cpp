#include "Editor/EntityOutline.h"
#include "Editor/Logger.h"
#include "RendererFrontend.h"
#include "Material.h"

#include "DirectXMath.h"
#include <cassert>
#include <variant>

#include "imgui.h"


namespace dx = DirectX;

EntityOutline::EntityOutline()
{
}

EntityOutline::~EntityOutline()
{
}


void EntityOutline::RenderEProperties(RenderableEntity& entity, LightInfo& lightInfo)
{
	constexpr float oePi = 180.0f * dx::XM_1DIVPI;
	constexpr float invOePi = 1.0f / oePi;
	entity.localWorldTransf.rot.x *= oePi;
	entity.localWorldTransf.rot.y *= oePi;
	entity.localWorldTransf.rot.z *= oePi;

	ImGui::SeparatorText("Transform");
	{
		ImGui::DragFloat3("Position", &entity.localWorldTransf.pos.x, 0.01f);
		ImGui::DragFloat3("Rotation", &entity.localWorldTransf.rot.x, 0.1f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
		ImGui::DragFloat3("Scale", &entity.localWorldTransf.scal.x, 0.01f, 0.0f, 0.0f, "%.3f");
	}

	entity.localWorldTransf.rot.x *= invOePi;
	entity.localWorldTransf.rot.y *= invOePi;
	entity.localWorldTransf.rot.z *= invOePi;

}

void EntityOutline::RenderEProperties(RenderableEntity& entity, SubmeshInfo* smInfo, MaterialDesc* matDesc)
{
	constexpr float oePi = 180.0f * dx::XM_1DIVPI;
	constexpr float invOePi = 1.0f / oePi;
	entity.localWorldTransf.rot.x *= oePi;
	entity.localWorldTransf.rot.y *= oePi;
	entity.localWorldTransf.rot.z *= oePi;

	ImGui::SeparatorText("Transform");
	{
		ImGui::DragFloat3("Position", &entity.localWorldTransf.pos.x, 0.01f);
		ImGui::DragFloat3("Rotation", &entity.localWorldTransf.rot.x, 0.1f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
		ImGui::DragFloat3("Scale", &entity.localWorldTransf.scal.x, 0.01f, 0.0f, 0.0f, "%.3f");
	}

	entity.localWorldTransf.rot.x *= invOePi;
	entity.localWorldTransf.rot.y *= invOePi;
	entity.localWorldTransf.rot.z *= invOePi;

	//TODO: better check if entity has mesh; probably move check to callee of this function
	if (smInfo != nullptr) {
		assert(matDesc != nullptr);
		ImGui::SeparatorText("Material");
		{

			ImGui::ColorEdit4("Base Color", &matDesc->baseColor.x);
			if (ImGui::IsItemEdited()) {
				smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
			}

			ImGui::ColorEdit4("Subsurface Albedo", &matDesc->subsurfaceAlbedo.x);
			if (ImGui::IsItemEdited()) {
				smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
			}

			ImGui::DragFloat4("Mean Free Path Color", &matDesc->meanFreePathColor.x, 0.01f, 0.001, ImGuiSliderFlags_AlwaysClamp);
			if (ImGui::IsItemEdited()) {
				smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
			}
			
			ImGui::DragFloat("Mean Free Path Distance", &matDesc->meanFreePathDistance, 0.0001f, 0.0001f, 0.0f, "%.4f", ImGuiSliderFlags_NoRoundToFormat | ImGuiSliderFlags_AlwaysClamp);
			if (ImGui::IsItemEdited()) {
				smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
			}

			ImGui::DragFloat("scale", &matDesc->scale, 1.0f, 1.0f, 16.0f);
			if (ImGui::IsItemEdited()) {
				smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
			}

			wcstombs(tmpCharBuff, matDesc->albedoPath, 128);
			ImGui::InputText("Albedo Map", tmpCharBuff, 128);
			if (ImGui::IsItemDeactivatedAfterEdit()) {
				mbstowcs(matDesc->albedoPath, tmpCharBuff, 128);
				smInfo->matCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
			}

			ImGui::Spacing();
			ImGui::Spacing();

			wcstombs(tmpCharBuff, matDesc->normalPath, 128);
			ImGui::InputText("Normal Map", tmpCharBuff, 128);
			if (ImGui::IsItemDeactivatedAfterEdit()) {
				mbstowcs(matDesc->normalPath, tmpCharBuff, 128);
				smInfo->matCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
			}

			ImGui::Spacing();
			ImGui::Spacing();
			
			{
				static int smoothnessMode = 0;

				if ((matDesc->unionTags & SMOOTHNESS_IS_MAP) == SMOOTHNESS_IS_MAP) {
					smoothnessMode = 1;
				}
				else {
					smoothnessMode = 0;
				}

				if (ImGui::RadioButton("Value", &smoothnessMode, 0)) {
					//Logger::AddLog(Logger::Channel::TRACE, "Clicked Radio V\n");
					matDesc->smoothnessValue = 0.0f;
					matDesc->unionTags &= ~SMOOTHNESS_IS_MAP;
					smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
				}
				ImGui::SameLine();
				if (ImGui::RadioButton("Texture", &smoothnessMode, 1)) {
					//Logger::AddLog(Logger::Channel::TRACE, "Clicked Radio T\n");
					mbstowcs(matDesc->smoothnessMap, "\0", 128);
					matDesc->unionTags |= SMOOTHNESS_IS_MAP;
					smInfo->matCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
				}

				if (smoothnessMode == 0) {
					ImGui::SliderFloat("Smoothness", &(matDesc->smoothnessValue), 0, 1);
					if (ImGui::IsItemEdited()) {
						smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
					}
				}
				else {
					wcstombs(tmpCharBuff, matDesc->smoothnessMap, 128);
					ImGui::InputText("Smoothness Map", tmpCharBuff, 128);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						mbstowcs(matDesc->smoothnessMap, tmpCharBuff, 128);
						smInfo->matCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
					}
				}
			}


			ImGui::Spacing();
			ImGui::Spacing();

			if (ImGui::BeginChild("metalness")) {
				static int metalnessMode = 0;

				if ((matDesc->unionTags & METALNESS_IS_MAP) == METALNESS_IS_MAP) {
					metalnessMode = 1;
				}
				else {
					metalnessMode = 0;
				}

				if (ImGui::RadioButton("Value", &metalnessMode, 0)) {
					//Logger::AddLog(Logger::Channel::TRACE, "Clicked Radio V %3d\n", metalnessMode);
					matDesc->metalnessValue = 0.0f;
					matDesc->unionTags &= ~METALNESS_IS_MAP;
					smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
				}
				ImGui::SameLine();
				if (ImGui::RadioButton("Texture", &metalnessMode, 1)) {
					//Logger::AddLog(Logger::Channel::TRACE, "Clicked Radio T %3d\n", metalnessMode);
					mbstowcs(matDesc->metalnessMap, "\0", 128);
					matDesc->unionTags |= METALNESS_IS_MAP;
					smInfo->matCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
				}

				if (metalnessMode == 0) {
					ImGui::SliderFloat("Metalness", &matDesc->metalnessValue, 0, 1);
					if (ImGui::IsItemEdited()) {
						smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
					}
				}
				else {
					wcstombs(tmpCharBuff, matDesc->metalnessMap, 128);
					ImGui::InputTextWithHint("Metalness Mask", "Insert Path", tmpCharBuff, 128);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						mbstowcs(matDesc->metalnessMap, tmpCharBuff, 128);
						smInfo->matCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
					}
				}
			}
			ImGui::EndChild();
			

		}
	}

}
