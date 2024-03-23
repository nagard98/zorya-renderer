#include "editor/EntityOutline.h"
#include "editor/Logger.h"
#include "Editor.h"

#include "renderer/frontend/RendererFrontend.h"
#include "renderer/frontend/Material.h"

#include "reflection/Reflection.h"
#include "reflection/Reflection_auto_generated.h"

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

template<zorya::VAR_REFL_TYPE T>
bool RenderEPropertyImpl(const char* structAddress, const MemberMeta& memMeta) {
	Logger::AddLog(Logger::Channel::WARNING, "type %s not supported in editor\n", zorya::VAR_REFL_TYPE_STRING[memMeta.type]);
	return false;
}

template<>
bool RenderEPropertyImpl<zorya::VAR_REFL_TYPE::FLOAT>(const char* structAddress, const MemberMeta& memMeta) {
	ImGui::DragFloat(memMeta.name, (float*)(structAddress + memMeta.offset), 0.01f, 0.001f, 0.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
	return ImGui::IsItemEdited();
}

template<>
bool RenderEPropertyImpl<zorya::VAR_REFL_TYPE::INT>(const char* structAddress, const MemberMeta& memMeta) {
	ImGui::DragInt(memMeta.name, (int*)(structAddress + memMeta.offset), 0, 0, 1, "%d", ImGuiSliderFlags_AlwaysClamp);
	return ImGui::IsItemEdited();
}

template<>
bool RenderEPropertyImpl<zorya::VAR_REFL_TYPE::UINT8>(const char* structAddress, const MemberMeta& memMeta) {
	ImGui::DragInt(memMeta.name, (int*)(structAddress + memMeta.offset), 0, 0, 1, "%d", ImGuiSliderFlags_AlwaysClamp);
	return ImGui::IsItemEdited();
}

template<>
bool RenderEPropertyImpl<zorya::VAR_REFL_TYPE::UINT16>(const char* structAddress, const MemberMeta& memMeta) {
	ImGui::DragInt(memMeta.name, (int*)(structAddress + memMeta.offset), 0, 0, 1, "%d", ImGuiSliderFlags_AlwaysClamp);
	return ImGui::IsItemEdited();
}

template<>
bool RenderEPropertyImpl<zorya::VAR_REFL_TYPE::UINT32>(const char* structAddress, const MemberMeta& memMeta) {
	ImGui::DragInt(memMeta.name, (int*)(structAddress + memMeta.offset), 0, 0, 1, "%d", ImGuiSliderFlags_AlwaysClamp);
	return ImGui::IsItemEdited();
}

template<>
bool RenderEPropertyImpl<zorya::VAR_REFL_TYPE::UINT64>(const char* structAddress, const MemberMeta& memMeta) {
	ImGui::DragInt(memMeta.name, (int*)(structAddress + memMeta.offset), 0, 0, 1, "%d", ImGuiSliderFlags_AlwaysClamp);
	return ImGui::IsItemEdited();
}

template<>
bool RenderEPropertyImpl<zorya::VAR_REFL_TYPE::XMFLOAT2>(const char* structAddress, const MemberMeta& memMeta) {
	ImGui::DragFloat2(memMeta.name, (float*)(structAddress + memMeta.offset), 0.01f, 0.001f, 0.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
	return ImGui::IsItemEdited();
}

template<>
bool RenderEPropertyImpl<zorya::VAR_REFL_TYPE::XMFLOAT3>(const char* structAddress, const MemberMeta& memMeta) {
	ImGui::DragFloat3(memMeta.name, (float*)(structAddress + memMeta.offset), 0.01f, 0.001f, 0.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
	return ImGui::IsItemEdited();
}

template<>
bool RenderEPropertyImpl<zorya::VAR_REFL_TYPE::XMFLOAT4>(const char* structAddress, const MemberMeta& memMeta) {
	ImGui::DragFloat4(memMeta.name, (float*)(structAddress + memMeta.offset), 0.01f, 0.001f, 0.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
	return ImGui::IsItemEdited();
}


template<>
bool RenderEPropertyImpl<zorya::VAR_REFL_TYPE::WCHAR>(const char* structAddress, const MemberMeta& memMeta) {
	ImGui::Spacing();
	size_t numConvertedChars = 0;

	wcstombs_s(&numConvertedChars, EntityOutline::tmpCharBuff, 128, (wchar_t*)(structAddress + memMeta.offset), 128);
	ImGui::InputText(memMeta.name, EntityOutline::tmpCharBuff, 128);
	mbstowcs_s(&numConvertedChars, (wchar_t*)(structAddress + memMeta.offset), 128, EntityOutline::tmpCharBuff, 128);
	bool isEditingComplete = ImGui::IsItemDeactivatedAfterEdit();

	ImGui::PushID((void*)(structAddress + memMeta.offset));
	ImGuiID idCurrentDialog = ImGui::GetItemID();

	if (ImGui::Button("Import")) {
		zorya::fileBrowser.Open();
		EntityOutline::idDialogOpen = idCurrentDialog;
	}

	if (EntityOutline::idDialogOpen == idCurrentDialog && zorya::fileBrowser.HasSelected()) {
		std::wstring importFilepath = zorya::fileBrowser.GetSelected().wstring();
		wcsncpy_s((wchar_t*)(structAddress + memMeta.offset), 128, importFilepath.c_str(), wcsnlen_s(importFilepath.c_str(), MAX_PATH));
		zorya::fileBrowser.ClearSelected();
		isEditingComplete = true;
	}

	ImGui::PopID();

	ImGui::Spacing();

	return isEditingComplete;
}


bool RenderEProperty(const char* structAddress, const MemberMeta& memMeta) {
	switch (memMeta.type) {
	case zorya::VAR_REFL_TYPE::FLOAT:
		return RenderEPropertyImpl<zorya::VAR_REFL_TYPE::FLOAT>(structAddress, std::forward<const MemberMeta>(memMeta));
	case zorya::VAR_REFL_TYPE::INT:
		return RenderEPropertyImpl<zorya::VAR_REFL_TYPE::INT>(structAddress, std::forward<const MemberMeta>(memMeta));
	case zorya::VAR_REFL_TYPE::UINT8:
		return RenderEPropertyImpl<zorya::VAR_REFL_TYPE::UINT8>(structAddress, std::forward<const MemberMeta>(memMeta));
	case zorya::VAR_REFL_TYPE::UINT16:
		return RenderEPropertyImpl<zorya::VAR_REFL_TYPE::UINT16>(structAddress, std::forward<const MemberMeta>(memMeta));
	case zorya::VAR_REFL_TYPE::UINT32:
		return RenderEPropertyImpl<zorya::VAR_REFL_TYPE::UINT32>(structAddress, std::forward<const MemberMeta>(memMeta));
	case zorya::VAR_REFL_TYPE::UINT64:
		return RenderEPropertyImpl<zorya::VAR_REFL_TYPE::UINT64>(structAddress, std::forward<const MemberMeta>(memMeta));
	case zorya::VAR_REFL_TYPE::XMFLOAT2:
		return RenderEPropertyImpl<zorya::VAR_REFL_TYPE::XMFLOAT2>(structAddress, std::forward<const MemberMeta>(memMeta));
	case zorya::VAR_REFL_TYPE::XMFLOAT3:
		return RenderEPropertyImpl<zorya::VAR_REFL_TYPE::XMFLOAT3>(structAddress, std::forward<const MemberMeta>(memMeta));
	case zorya::VAR_REFL_TYPE::XMFLOAT4:
		return RenderEPropertyImpl<zorya::VAR_REFL_TYPE::XMFLOAT4>(structAddress, std::forward<const MemberMeta>(memMeta));
	case zorya::VAR_REFL_TYPE::WCHAR:
		return RenderEPropertyImpl<zorya::VAR_REFL_TYPE::WCHAR>(structAddress, std::forward<const MemberMeta>(memMeta));
	default:
		return RenderEPropertyImpl<zorya::VAR_REFL_TYPE::NOT_SUPPORTED>(structAddress, std::forward<const MemberMeta>(memMeta));
	}
}

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
			foreachfield(&lightInfo.dirLight, +[](const char* structAddr, const MemberMeta& memMeta) -> std::uint8_t {
				bool isEdited = RenderEProperty(structAddr, std::forward<const MemberMeta>(memMeta));
				return isEdited;
				});
			break;
		
		case LightType::SPOT:
			foreachfield(&lightInfo.spotLight, +[](const char* structAddr, const MemberMeta& memMeta) -> std::uint8_t {
				bool isEdited = RenderEProperty(structAddr, std::forward<const MemberMeta>(memMeta));
				return isEdited;
				});
			break;

		case LightType::POINT:
			foreachfield(&lightInfo.pointLight, +[](const char* structAddr, const MemberMeta& memMeta) -> std::uint8_t {
				bool isEdited = RenderEProperty(structAddr, std::forward<const MemberMeta>(memMeta));
				return isEdited;
				});
			break;

		}

	}

}

void EntityOutline::RenderEProperties(RenderableEntity& entity, SubmeshInfo* smInfo, ReflectionBase* matDesc)
{
	RenderETransform(entity);

	//TODO: better check if entity has mesh; probably move check to callee of this function
	if (smInfo != nullptr) {
		assert(matDesc != nullptr);
		ImGui::SeparatorText("Material");
		{
			//size_t numConvertedChars = 0;

			smInfo->matCacheHnd.isCached = matDesc->foreachreflectedfield(+[](const char* structAddr, const MemberMeta& memMeta) -> std::uint8_t {
				bool isEdited = RenderEProperty(structAddr, std::forward<const MemberMeta>(memMeta));

				if (isEdited) {
					if(memMeta.type == zorya::VAR_REFL_TYPE::WCHAR) return (UPDATE_MAT_MAPS | UPDATE_MAT_PRMS);
					else return UPDATE_MAT_PRMS;
				}
				else {
					return 0;
				}
				});

			zorya::fileBrowser.Display();

			//./assets/brocc-the-athlete/textures/Sporter_Albedo.png
			//{
			//	static int smoothnessMode = 0;
			//	

			//	if ((matDesc->unionTags & SMOOTHNESS_IS_MAP) == SMOOTHNESS_IS_MAP) {
			//		smoothnessMode = 1;
			//	}
			//	else {
			//		smoothnessMode = 0;
			//	}

			//	if (ImGui::RadioButton("Value", &smoothnessMode, 0)) {
			//		//Logger::AddLog(Logger::Channel::TRACE, "Clicked Radio V\n");
			//		matDesc->smoothnessValue = 0.0f;
			//		matDesc->unionTags &= ~SMOOTHNESS_IS_MAP;
			//		smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
			//	}
			//	ImGui::SameLine();
			//	if (ImGui::RadioButton("Texture", &smoothnessMode, 1)) {
			//		//Logger::AddLog(Logger::Channel::TRACE, "Clicked Radio T\n");
			//		mbstowcs_s(&numConvertedChars, matDesc->smoothnessMap, "\0", 128);
			//		matDesc->unionTags |= SMOOTHNESS_IS_MAP;
			//		smInfo->matCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
			//	}

			//	if (smoothnessMode == 0) {
			//		ImGui::SliderFloat("Smoothness", &(matDesc->smoothnessValue), 0, 1);
			//		if (ImGui::IsItemEdited()) {
			//			smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
			//		}
			//	}
			//	else {
			//		wcstombs_s(&numConvertedChars, tmpCharBuff, matDesc->smoothnessMap, 128);
			//		ImGui::InputText("Smoothness Map", tmpCharBuff, 128);
			//		if (ImGui::IsItemDeactivatedAfterEdit()) {
			//			mbstowcs_s(&numConvertedChars, matDesc->smoothnessMap, tmpCharBuff, 128);
			//			smInfo->matCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
			//		}
			//	}
			//}


			ImGui::Spacing();
			ImGui::Spacing();

			//if (ImGui::BeginChild("metalness")) {
			//	static int metalnessMode = 0;

			//	if ((matDesc->unionTags & METALNESS_IS_MAP) == METALNESS_IS_MAP) {
			//		metalnessMode = 1;
			//	}
			//	else {
			//		metalnessMode = 0;
			//	}

			//	if (ImGui::RadioButton("Value", &metalnessMode, 0)) {
			//		//Logger::AddLog(Logger::Channel::TRACE, "Clicked Radio V %3d\n", metalnessMode);
			//		matDesc->metalnessValue = 0.0f;
			//		matDesc->unionTags &= ~METALNESS_IS_MAP;
			//		smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
			//	}
			//	ImGui::SameLine();
			//	if (ImGui::RadioButton("Texture", &metalnessMode, 1)) {
			//		//Logger::AddLog(Logger::Channel::TRACE, "Clicked Radio T %3d\n", metalnessMode);
			//		mbstowcs_s(&numConvertedChars, matDesc->metalnessMap, "\0", 128);
			//		matDesc->unionTags |= METALNESS_IS_MAP;
			//		smInfo->matCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
			//	}

			//	if (metalnessMode == 0) {
			//		ImGui::SliderFloat("Metalness", &matDesc->metalnessValue, 0, 1);
			//		if (ImGui::IsItemEdited()) {
			//			smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
			//		}
			//	}
			//	else {
			//		wcstombs_s(&numConvertedChars, tmpCharBuff, matDesc->metalnessMap, 128);
			//		ImGui::InputTextWithHint("Metalness Mask", "Insert Path", tmpCharBuff, 128);
			//		if (ImGui::IsItemDeactivatedAfterEdit()) {
			//			mbstowcs_s(&numConvertedChars, matDesc->metalnessMap, tmpCharBuff, 128);
			//			smInfo->matCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
			//		}
			//	}
			//}
			//ImGui::EndChild();
			//

		}
	}

}
