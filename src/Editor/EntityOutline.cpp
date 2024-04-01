#include "editor/EntityOutline.h"
#include "editor/Logger.h"
#include "Editor.h"

#include "renderer/frontend/RendererFrontend.h"
#include "renderer/frontend/Material.h"

#include "reflection/Reflection.h"
//#include "reflection/Reflection_auto_generated.h"

#include "DirectXMath.h"
#include <cassert>
#include <variant>

#include "imgui.h"
#include "imfilebrowser.h"


namespace dx = DirectX;

EntityOutline::EntityOutline()
{
}

EntityOutline::~EntityOutline()
{
}

char EntityOutline::tmpCharBuff[128];
ImGuiID EntityOutline::idDialogOpen = 0;


template<typename T>
bool RenderEProperty(T* fieldAddr, const char* name) {
	bool modified = false;

	ImGui::SeparatorText(name); 
	{
		foreachfield([&](auto& reflField, auto& fieldMeta) {
			bool isEdited = RenderEProperty(fieldMeta.type.castTo((void*)GET_FIELD_ADDRESS(reflField, fieldMeta.offset)), fieldMeta.name);
			if (isEdited) {
				modified = UPDATE_MAT_PRMS;
			}
			}, *fieldAddr);
	}

	//Logger::AddLog(Logger::Channel::WARNING, "type %s not supported in editor\n", zorya::VAR_REFL_TYPE_STRING[memMeta.typeEnum]);
	return modified;
}

template<>
bool RenderEProperty(float* fieldAddr, const char* name) {
	ImGui::DragFloat(name, fieldAddr, 0.01f, 0.001f, 0.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
	return ImGui::IsItemEdited();
}

template<>
bool RenderEProperty(int* fieldAddr, const char* name) {
	ImGui::DragInt(name, fieldAddr, 0, 0, 1, "%d", ImGuiSliderFlags_AlwaysClamp);
	return ImGui::IsItemEdited();
}

template<>
bool RenderEProperty(uint8_t* fieldAddr, const char* name) {
	ImGui::DragInt(name, (int*)fieldAddr, 0, 0, 1, "%d", ImGuiSliderFlags_AlwaysClamp);
	return ImGui::IsItemEdited();
}

template<>
bool RenderEProperty(uint16_t* fieldAddr, const char* name) {
	ImGui::DragInt(name, (int*)fieldAddr, 0, 0, 1, "%d", ImGuiSliderFlags_AlwaysClamp);
	return ImGui::IsItemEdited();
}

template<>
bool RenderEProperty(uint32_t* fieldAddr, const char* name) {
	ImGui::DragInt(name, (int*)fieldAddr, 0, 0, 1, "%d", ImGuiSliderFlags_AlwaysClamp);
	return ImGui::IsItemEdited();
}

template<>
bool RenderEProperty(uint64_t* fieldAddr, const char* name) {
	ImGui::DragInt(name, (int*)fieldAddr, 0, 0, 1, "%d", ImGuiSliderFlags_AlwaysClamp);
	return ImGui::IsItemEdited();
}

template<>
bool RenderEProperty(dx::XMFLOAT2* fieldAddr, const char* name) {
	ImGui::DragFloat2(name, &fieldAddr->x, 0.01f, 0.001f, 0.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
	return ImGui::IsItemEdited();
}

template<>
bool RenderEProperty(dx::XMFLOAT3* fieldAddr, const char* name) {
	ImGui::DragFloat3(name, &fieldAddr->x, 0.01f, 0.001f, 0.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
	return ImGui::IsItemEdited();
}

template<>
bool RenderEProperty(dx::XMFLOAT4* fieldAddr, const char* name) {
	ImGui::DragFloat4(name, &fieldAddr->x, 0.01f, 0.001f, 0.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
	return ImGui::IsItemEdited();
}

template<>
bool RenderEProperty(wchar_t* fieldAddr, const char* name) {
	ImGui::Spacing();
	size_t numConvertedChars = 0;

	wcstombs_s(&numConvertedChars, EntityOutline::tmpCharBuff, 128, fieldAddr, 128);
	ImGui::InputText(name, EntityOutline::tmpCharBuff, 128);
	mbstowcs_s(&numConvertedChars, fieldAddr, 128, EntityOutline::tmpCharBuff, 128);
	bool isEditingComplete = ImGui::IsItemDeactivatedAfterEdit();

	ImGui::PushID((void*)fieldAddr);
	ImGuiID idCurrentDialog = ImGui::GetItemID();

	if (ImGui::Button("Import")) {
		zorya::fileBrowser.Open();
		EntityOutline::idDialogOpen = idCurrentDialog;
	}

	if (EntityOutline::idDialogOpen == idCurrentDialog && zorya::fileBrowser.HasSelected()) {
		std::wstring importFilepath = zorya::fileBrowser.GetSelected().wstring();
		wcsncpy_s(fieldAddr, 128, importFilepath.c_str(), wcsnlen_s(importFilepath.c_str(), MAX_PATH));
		zorya::fileBrowser.ClearSelected();
		isEditingComplete = true;
	}

	ImGui::PopID();

	ImGui::Spacing();

	return isEditingComplete;
}


template <typename T>
struct RenderProperty {
	static void render(T& stru) {
		OutputDebugString("rendering struct\n");
	}
};


void EntityOutline::RenderETransform(RenderableEntity& entity) {
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


void EntityOutline::RenderEProperties(RenderableEntity& entity, LightInfo& lightInfo)
{
	RenderETransform(entity);

	ImGui::SeparatorText("Light Parameters");
	{
		switch (lightInfo.tag) {

		case LightType::DIRECTIONAL:
			foreachfield([](auto& structAddr, auto& fieldMeta) {
				RenderEProperty(fieldMeta.type.castTo((void*)GET_FIELD_ADDRESS(structAddr, fieldMeta.offset)), fieldMeta.name);
				}, lightInfo.dirLight);
			break;
		
		case LightType::SPOT:
			foreachfield([](auto& structAddr, auto& fieldMeta){
				RenderEProperty(fieldMeta.type.castTo((void*)GET_FIELD_ADDRESS(structAddr, fieldMeta.offset)), fieldMeta.name);
				}, lightInfo.spotLight);
			break;

		case LightType::POINT:
			foreachfield([](auto& structAddr, auto& fieldMeta) {
				RenderEProperty(fieldMeta.type.castTo((void*)GET_FIELD_ADDRESS(structAddr, fieldMeta.offset)), fieldMeta.name);
				}, lightInfo.pointLight);
			break;

		}

	}

}

bool importDialog(wchar_t* path, const char* id) {
	ImGui::PushID(id);
	ImGuiID idCurrentDialog = ImGui::GetItemID();

	if (ImGui::Button("Import")) {
		zorya::fileBrowser.Open();
		EntityOutline::idDialogOpen = idCurrentDialog;
	}

	bool isEditingComplete = false;

	if (EntityOutline::idDialogOpen == idCurrentDialog && zorya::fileBrowser.HasSelected()) {
		std::wstring importFilepath = zorya::fileBrowser.GetSelected().wstring();
		wcsncpy_s(path, 128, importFilepath.c_str(), wcsnlen_s(importFilepath.c_str(), MAX_PATH));
		zorya::fileBrowser.ClearSelected();
		isEditingComplete = true;
	}

	ImGui::PopID();

	return isEditingComplete;
}



void EntityOutline::RenderEProperties(RenderableEntity& entity, SubmeshInfo* smInfo, ReflectionBase* matDesc)
{
	RenderETransform(entity);

	//TODO: better check if entity has mesh; probably move check to callee of this function
	if (smInfo != nullptr) {
		assert(matDesc != nullptr);
		//auto& matDesc2 = static_cast<ReflectionContainer<StandardMaterialDesc>*>(const_cast<ReflectionBase*>(matDesc))->reflectedStruct;
		auto& matDesc2 = static_cast<ReflectionContainer<StandardMaterialDesc>*>(const_cast<ReflectionBase*>(matDesc))->reflectedStruct;

		ImGui::SeparatorText("Material");
		{
			size_t numConvertedChars = 0;

			//foreachfield([&](auto& structAddr, auto& fieldMeta) {
			//	bool isEdited = RenderEProperty(fieldMeta.type.castTo((void*)GET_FIELD_ADDRESS(structAddr, fieldMeta.offset)), fieldMeta.name);
			//	if (isEdited) {
			//		smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
			//	}
			//}, matDesc2);


			zorya::fileBrowser.Display();

			ImGui::ColorEdit4("Base Color", &matDesc2.baseColor.x);
			if (ImGui::IsItemEdited()) {
				smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
			}

			wcstombs(tmpCharBuff, matDesc2.albedoPath, 128);
			ImGui::InputTextWithHint("Albedo Map", "Texture Path", tmpCharBuff, 128);
			if (ImGui::IsItemDeactivatedAfterEdit()) {
				mbstowcs(matDesc2.albedoPath, tmpCharBuff, 128);
				smInfo->matCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
			}

			if (importDialog(matDesc2.albedoPath, "import_albedo")) {
				smInfo->matCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
			}

			ImGui::Spacing();
			ImGui::Spacing();

			wcstombs(tmpCharBuff, matDesc2.normalPath, 128);
			ImGui::InputTextWithHint("Normal Map", "Texture Path", tmpCharBuff, 128);
			if (ImGui::IsItemDeactivatedAfterEdit()) {
				mbstowcs(matDesc2.normalPath, tmpCharBuff, 128);
				smInfo->matCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
			}

			if (importDialog(matDesc2.normalPath, "import_normal")) {
				smInfo->matCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
			}

			ImGui::Spacing();
			ImGui::Spacing();

			//./assets/brocc-the-athlete/textures/Sporter_Albedo.png
			{
				static int smoothnessMode = 0;
				

				if ((matDesc2.unionTags & SMOOTHNESS_IS_MAP) == SMOOTHNESS_IS_MAP) {
					smoothnessMode = 1;
				}
				else {
					smoothnessMode = 0;
				}

				if (ImGui::RadioButton("Value", &smoothnessMode, 0)) {
					//Logger::AddLog(Logger::Channel::TRACE, "Clicked Radio V\n");
					matDesc2.smoothnessValue = 0.0f;
					matDesc2.unionTags &= ~SMOOTHNESS_IS_MAP;
					smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
				}
				ImGui::SameLine();
				if (ImGui::RadioButton("Texture", &smoothnessMode, 1)) {
					//Logger::AddLog(Logger::Channel::TRACE, "Clicked Radio T\n");
					mbstowcs_s(&numConvertedChars, matDesc2.smoothnessMap, "\0", 128);
					matDesc2.unionTags |= SMOOTHNESS_IS_MAP;
					smInfo->matCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
				}

				if (smoothnessMode == 0) {
					ImGui::SliderFloat("Smoothness", &(matDesc2.smoothnessValue), 0, 1);
					if (ImGui::IsItemEdited()) {
						smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
					}
				}
				else {
					wcstombs_s(&numConvertedChars, tmpCharBuff, matDesc2.smoothnessMap, 128);
					ImGui::InputTextWithHint("Smoothness Map", "Texture Path", tmpCharBuff, 128);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						mbstowcs_s(&numConvertedChars, matDesc2.smoothnessMap, tmpCharBuff, 128);
						smInfo->matCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
					}

					if (importDialog(matDesc2.smoothnessMap, "import_smoothness")) {
						smInfo->matCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
					}
				}
			}


			ImGui::Spacing();
			ImGui::Spacing();

			/*if (ImGui::BeginChild("metalness"))*/ {
				static int metalnessMode = 0;

				if ((matDesc2.unionTags & METALNESS_IS_MAP) == METALNESS_IS_MAP) {
					metalnessMode = 1;
				}
				else {
					metalnessMode = 0;
				}

				if (ImGui::RadioButton("Value", &metalnessMode, 0)) {
					//Logger::AddLog(Logger::Channel::TRACE, "Clicked Radio V %3d\n", metalnessMode);
					matDesc2.metalnessValue = 0.0f;
					matDesc2.unionTags &= ~METALNESS_IS_MAP;
					smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
				}
				ImGui::SameLine();
				if (ImGui::RadioButton("Texture", &metalnessMode, 1)) {
					//Logger::AddLog(Logger::Channel::TRACE, "Clicked Radio T %3d\n", metalnessMode);
					mbstowcs_s(&numConvertedChars, matDesc2.metalnessMap, "\0", 128);
					matDesc2.unionTags |= METALNESS_IS_MAP;
					smInfo->matCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
				}

				if (metalnessMode == 0) {
					ImGui::SliderFloat("Metalness", &matDesc2.metalnessValue, 0, 1);
					if (ImGui::IsItemEdited()) {
						smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
					}
				}
				else {
					wcstombs_s(&numConvertedChars, tmpCharBuff, matDesc2.metalnessMap, 128);
					ImGui::InputTextWithHint("Metalness Mask", "Texture Path", tmpCharBuff, 128);
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						mbstowcs_s(&numConvertedChars, matDesc2.metalnessMap, tmpCharBuff, 128);
						smInfo->matCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
					}

					if (importDialog(matDesc2.metalnessMap, "import_metalness")) {
						smInfo->matCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
					}
				}
			}
			//ImGui::EndChild();
			
			ImGui::Spacing();
			ImGui::Spacing();

			ImGui::SeparatorText("SSS");
			{
				const char* sssOptions[] = {"No SSS", "Jimenez Gaussian" , "Jimenez Separable" , "Golubev" };
				int selected = (int)(matDesc2.selectedSSSModel);
				if (ImGui::Combo("SSS Model", &selected, sssOptions, IM_ARRAYSIZE(sssOptions))) {
					smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
					matDesc2.selectedSSSModel = (SSS_MODEL)selected;
				}


				ImGui::Spacing();

				switch (matDesc2.selectedSSSModel) {
				case SSS_MODEL::JIMENEZ_GAUSS:
				{
					ImGui::DragFloat("Mean Free Path Distance", &matDesc2.sssModel.meanFreePathDistance, 0.001f, 0.001f, 1000.0f, "%.4f");
					if (ImGui::IsItemEdited()) {
						smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
					}

					ImGui::DragFloat("Scale", &matDesc2.sssModel.scale, 0.001f, 0.001f, 1000.0f, "%.4f");
					if (ImGui::IsItemEdited()) {
						smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
					}

					break;
				}
				case SSS_MODEL::JIMENEZ_SEPARABLE:
				{
					ImGui::DragFloat("Mean Free Path Distance", &matDesc2.sssModel.meanFreePathDistance, 0.001f, 0.001f, 1000.0f, "%.4f");
					if (ImGui::IsItemEdited()) {
						smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
					}

					ImGui::DragFloat("Scale", &matDesc2.sssModel.scale, 0.001f, 0.001f, 1000.0f, "%.4f");
					if (ImGui::IsItemEdited()) {
						smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
					}
					break;
				}
				case SSS_MODEL::GOLUBEV:
				{
					ImGui::ColorEdit3("Subsurface Albedo", &matDesc2.sssModel.subsurfaceAlbedo.x);
					if (ImGui::IsItemEdited()) {
						smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
					}
					ImGui::DragFloat3("Mean Free Path Color", &matDesc2.sssModel.meanFreePathColor.x, 0.001f, 0.001f, 1000.0f, "%.3f");
					if (ImGui::IsItemEdited()) {
						smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
					}
					ImGui::DragFloat("Mean Free Path Distance", &matDesc2.sssModel.meanFreePathDistance, 0.001f, 0.001f, 1000.0f, "%.4f");
					if (ImGui::IsItemEdited()) {
						smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
					}
					ImGui::DragFloat("Scale", &matDesc2.sssModel.scale, 0.001f, 0.001f, 1000.0f, "%.4f");
					if (ImGui::IsItemEdited()) {
						smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
					}
					int numSamples = matDesc2.sssModel.numSamples;
					ImGui::SliderInt("Num Samples", &numSamples, 4, 64);
					if (ImGui::IsItemEdited()) {
						matDesc2.sssModel.numSamples = numSamples;
						smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
					}
					break;
				}
				default:
					break;

				}
			}

		}



	}

}
