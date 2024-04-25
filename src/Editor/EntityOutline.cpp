#include "editor/EntityOutline.h"
#include "editor/Logger.h"
#include "Editor.h"

#include "renderer/frontend/RendererFrontend.h"
#include "renderer/frontend/Material.h"

#include "reflection/Reflection.h"

#include "DirectXMath.h"
#include <cassert>
#include <variant>

#include "imgui.h"
#include "imfilebrowser.h"

namespace zorya
{
	namespace dx = DirectX;

	Entity_Outline::Entity_Outline()
	{}

	Entity_Outline::~Entity_Outline()
	{}

	char Entity_Outline::tmp_char_buff[128];
	ImGuiID Entity_Outline::id_dialog_open = 0;


	template<typename T>
	bool render_entity_property(T* field_addr, const char* name)
	{
		bool modified = false;

		ImGui::SeparatorText(name);
		{
			for_each_field([&](auto& refl_field, auto& field_meta) {
				bool is_edited = render_entity_property(field_meta.type.cast_to((void*)GET_FIELD_ADDRESS(refl_field, field_meta.offset)), field_meta.name);
				if (is_edited)
				{
					modified = UPDATE_MAT_PRMS;
				}
				}, *field_addr);
		}

		//Logger::AddLog(Logger::Channel::WARNING, "type %s not supported in editor\n", zorya::VAR_REFL_TYPE_STRING[memMeta.typeEnum]);
		return modified;
	}

	template<>
	bool render_entity_property(float* field_addr, const char* name)
	{
		ImGui::DragFloat(name, field_addr, 0.01f, 0.001f, 0.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
		return ImGui::IsItemEdited();
	}

	template<>
	bool render_entity_property(int* field_addr, const char* name)
	{
		ImGui::DragInt(name, field_addr, 0, 0, 1, "%d", ImGuiSliderFlags_AlwaysClamp);
		return ImGui::IsItemEdited();
	}

	template<>
	bool render_entity_property(uint8_t* field_addr, const char* name)
	{
		ImGui::DragInt(name, (int*)field_addr, 0, 0, 1, "%d", ImGuiSliderFlags_AlwaysClamp);
		return ImGui::IsItemEdited();
	}

	template<>
	bool render_entity_property(uint16_t* field_addr, const char* name)
	{
		ImGui::DragInt(name, (int*)field_addr, 0, 0, 1, "%d", ImGuiSliderFlags_AlwaysClamp);
		return ImGui::IsItemEdited();
	}

	template<>
	bool render_entity_property(uint32_t* field_addr, const char* name)
	{
		ImGui::DragInt(name, (int*)field_addr, 0, 0, 1, "%d", ImGuiSliderFlags_AlwaysClamp);
		return ImGui::IsItemEdited();
	}

	template<>
	bool render_entity_property(uint64_t* field_addr, const char* name)
	{
		ImGui::DragInt(name, (int*)field_addr, 0, 0, 1, "%d", ImGuiSliderFlags_AlwaysClamp);
		return ImGui::IsItemEdited();
	}

	template<>
	bool render_entity_property(dx::XMFLOAT2* field_addr, const char* name)
	{
		ImGui::DragFloat2(name, &field_addr->x, 0.01f, 0.001f, 0.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
		return ImGui::IsItemEdited();
	}

	template<>
	bool render_entity_property(dx::XMFLOAT3* field_addr, const char* name)
	{
		ImGui::DragFloat3(name, &field_addr->x, 0.01f, 0.001f, 0.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
		return ImGui::IsItemEdited();
	}

	template<>
	bool render_entity_property(dx::XMFLOAT4* field_addr, const char* name)
	{
		ImGui::DragFloat4(name, &field_addr->x, 0.01f, 0.001f, 0.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
		return ImGui::IsItemEdited();
	}

	template<>
	bool render_entity_property(wchar_t* field_addr, const char* name)
	{
		ImGui::Spacing();
		size_t num_converted_chars = 0;

		wcstombs_s(&num_converted_chars, Entity_Outline::tmp_char_buff, 128, field_addr, 128);
		ImGui::InputText(name, Entity_Outline::tmp_char_buff, 128);
		mbstowcs_s(&num_converted_chars, field_addr, 128, Entity_Outline::tmp_char_buff, 128);
		bool is_editing_complete = ImGui::IsItemDeactivatedAfterEdit();

		ImGui::PushID((void*)field_addr);
		ImGuiID id_current_dialog = ImGui::GetItemID();

		if (ImGui::Button("Import"))
		{
			zorya::file_browser.Open();
			Entity_Outline::id_dialog_open = id_current_dialog;
		}

		if (Entity_Outline::id_dialog_open == id_current_dialog && zorya::file_browser.HasSelected())
		{
			std::wstring importFilepath = zorya::file_browser.GetSelected().wstring();
			wcsncpy_s(field_addr, 128, importFilepath.c_str(), wcsnlen_s(importFilepath.c_str(), MAX_PATH));
			zorya::file_browser.ClearSelected();
			is_editing_complete = true;
		}

		ImGui::PopID();

		ImGui::Spacing();

		return is_editing_complete;
	}


	template <typename T>
	struct RenderProperty
	{
		static void render(T& stru)
		{
			OutputDebugString("rendering struct\n");
		}
	};


	void Entity_Outline::render_entity_transform(Renderable_Entity& entity)
	{
		constexpr float oe_pi = 180.0f * dx::XM_1DIVPI;
		constexpr float inv_oe_pi = 1.0f / oe_pi;
		entity.local_world_transf.rot.x *= oe_pi;
		entity.local_world_transf.rot.y *= oe_pi;
		entity.local_world_transf.rot.z *= oe_pi;

		ImGui::SeparatorText("Transform");
		{
			ImGui::DragFloat3("Position", &entity.local_world_transf.pos.x, 0.01f);
			ImGui::DragFloat3("Rotation", &entity.local_world_transf.rot.x, 0.1f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_NoRoundToFormat);
			ImGui::DragFloat3("Scale", &entity.local_world_transf.scal.x, 0.01f, 0.0f, 0.0f, "%.3f");
		}

		entity.local_world_transf.rot.x *= inv_oe_pi;
		entity.local_world_transf.rot.y *= inv_oe_pi;
		entity.local_world_transf.rot.z *= inv_oe_pi;
	}


	void Entity_Outline::render_entity_properties(Renderable_Entity& entity, Light_Info& light_info)
	{
		render_entity_transform(entity);

		ImGui::SeparatorText("Light Parameters");
		{
			switch (light_info.tag)
			{

			case Light_Type::DIRECTIONAL:
				for_each_field([](auto& struct_addr, auto& field_meta) {
					render_entity_property(field_meta.type.cast_to((void*)GET_FIELD_ADDRESS(struct_addr, field_meta.offset)), field_meta.name);
					}, light_info.dir_light);
				break;

			case Light_Type::SPOT:
				for_each_field([](auto& struct_addr, auto& field_meta) {
					render_entity_property(field_meta.type.cast_to((void*)GET_FIELD_ADDRESS(struct_addr, field_meta.offset)), field_meta.name);
					}, light_info.spot_light);
				break;

			case Light_Type::POINT:
				for_each_field([](auto& struct_addr, auto& field_meta) {
					render_entity_property(field_meta.type.cast_to((void*)GET_FIELD_ADDRESS(struct_addr, field_meta.offset)), field_meta.name);
					}, light_info.point_light);
				break;

			}

		}

	}

	bool import_dialog(wchar_t* path, const char* id)
	{
		ImGui::PushID(id);
		ImGuiID id_current_dialog = ImGui::GetItemID();

		if (ImGui::Button("Import"))
		{
			zorya::file_browser.Open();
			Entity_Outline::id_dialog_open = id_current_dialog;
		}

		bool is_editing_complete = false;

		if (Entity_Outline::id_dialog_open == id_current_dialog && zorya::file_browser.HasSelected())
		{
			std::wstring import_file_path = zorya::file_browser.GetSelected().wstring();
			wcsncpy_s(path, 128, import_file_path.c_str(), wcsnlen_s(import_file_path.c_str(), MAX_PATH));
			zorya::file_browser.ClearSelected();
			is_editing_complete = true;
		}

		ImGui::PopID();

		return is_editing_complete;
	}



	void Entity_Outline::render_entity_properties(Renderable_Entity& entity, Submesh_Info* submesh_info, Reflection_Base* material_desc)
	{
		render_entity_transform(entity);

		//TODO: better check if entity has mesh; probably move check to callee of this function
		if (submesh_info != nullptr)
		{
			assert(material_desc != nullptr);
			//auto& matDesc2 = static_cast<ReflectionContainer<StandardMaterialDesc>*>(const_cast<ReflectionBase*>(matDesc))->reflectedStruct;
			auto& mat_desc_2 = static_cast<Reflection_Container<Standard_Material_Desc>*>(const_cast<Reflection_Base*>(material_desc))->reflected_struct;

			ImGui::SeparatorText("Material");
			{
				size_t num_converted_chars = 0;

				//foreachfield([&](auto& structAddr, auto& field_meta) {
				//	bool isEdited = RenderEProperty(field_meta.type.castTo((void*)GET_FIELD_ADDRESS(structAddr, field_meta.offset)), field_meta.name);
				//	if (isEdited) {
				//		smInfo->matCacheHnd.isCached = UPDATE_MAT_PRMS;
				//	}
				//}, matDesc2);


				zorya::file_browser.Display();

				ImGui::ColorEdit4("Base Color", &mat_desc_2.base_color.x);
				if (ImGui::IsItemEdited())
				{
					submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_PRMS;
				}

				wcstombs(tmp_char_buff, mat_desc_2.albedo_path, 128);
				ImGui::InputTextWithHint("Albedo Map", "Texture Path", tmp_char_buff, 128);
				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					mbstowcs(mat_desc_2.albedo_path, tmp_char_buff, 128);
					submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
				}

				if (import_dialog(mat_desc_2.albedo_path, "import_albedo"))
				{
					submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
				}

				ImGui::Spacing();
				ImGui::Spacing();

				wcstombs(tmp_char_buff, mat_desc_2.normal_path, 128);
				ImGui::InputTextWithHint("Normal Map", "Texture Path", tmp_char_buff, 128);
				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					mbstowcs(mat_desc_2.normal_path, tmp_char_buff, 128);
					submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
				}

				if (import_dialog(mat_desc_2.normal_path, "import_normal"))
				{
					submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
				}

				ImGui::Spacing();
				ImGui::Spacing();

				//./assets/brocc-the-athlete/textures/Sporter_Albedo.png
				{
					static int smoothness_mode = 0;


					if ((mat_desc_2.union_tags & SMOOTHNESS_IS_MAP) == SMOOTHNESS_IS_MAP)
					{
						smoothness_mode = 1;
					}
					else
					{
						smoothness_mode = 0;
					}

					if (ImGui::RadioButton("Value", &smoothness_mode, 0))
					{
						//Logger::AddLog(Logger::Channel::TRACE, "Clicked Radio V\n");
						mat_desc_2.smoothness_value = 0.0f;
						mat_desc_2.union_tags &= ~SMOOTHNESS_IS_MAP;
						submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_PRMS;
					}
					ImGui::SameLine();
					if (ImGui::RadioButton("Texture", &smoothness_mode, 1))
					{
						//Logger::AddLog(Logger::Channel::TRACE, "Clicked Radio T\n");
						mbstowcs_s(&num_converted_chars, mat_desc_2.smoothness_map, "\0", 128);
						mat_desc_2.union_tags |= SMOOTHNESS_IS_MAP;
						submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
					}

					if (smoothness_mode == 0)
					{
						ImGui::SliderFloat("Smoothness", &(mat_desc_2.smoothness_value), 0, 1);
						if (ImGui::IsItemEdited())
						{
							submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_PRMS;
						}
					}
					else
					{
						wcstombs_s(&num_converted_chars, tmp_char_buff, mat_desc_2.smoothness_map, 128);
						ImGui::InputTextWithHint("Smoothness Map", "Texture Path", tmp_char_buff, 128);
						if (ImGui::IsItemDeactivatedAfterEdit())
						{
							mbstowcs_s(&num_converted_chars, mat_desc_2.smoothness_map, tmp_char_buff, 128);
							submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
						}

						if (import_dialog(mat_desc_2.smoothness_map, "import_smoothness"))
						{
							submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
						}
					}
				}


				ImGui::Spacing();
				ImGui::Spacing();

				/*if (ImGui::BeginChild("metalness"))*/
				{
					static int metalness_mode = 0;

					if ((mat_desc_2.union_tags & METALNESS_IS_MAP) == METALNESS_IS_MAP)
					{
						metalness_mode = 1;
					}
					else
					{
						metalness_mode = 0;
					}

					if (ImGui::RadioButton("Value", &metalness_mode, 0))
					{
						//Logger::AddLog(Logger::Channel::TRACE, "Clicked Radio V %3d\n", metalnessMode);
						mat_desc_2.metalness_value = 0.0f;
						mat_desc_2.union_tags &= ~METALNESS_IS_MAP;
						submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_PRMS;
					}
					ImGui::SameLine();
					if (ImGui::RadioButton("Texture", &metalness_mode, 1))
					{
						//Logger::AddLog(Logger::Channel::TRACE, "Clicked Radio T %3d\n", metalnessMode);
						mbstowcs_s(&num_converted_chars, mat_desc_2.metalness_map, "\0", 128);
						mat_desc_2.union_tags |= METALNESS_IS_MAP;
						submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
					}

					if (metalness_mode == 0)
					{
						ImGui::SliderFloat("Metalness", &mat_desc_2.metalness_value, 0, 1);
						if (ImGui::IsItemEdited())
						{
							submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_PRMS;
						}
					}
					else
					{
						wcstombs_s(&num_converted_chars, tmp_char_buff, mat_desc_2.metalness_map, 128);
						ImGui::InputTextWithHint("Metalness Mask", "Texture Path", tmp_char_buff, 128);
						if (ImGui::IsItemDeactivatedAfterEdit())
						{
							mbstowcs_s(&num_converted_chars, mat_desc_2.metalness_map, tmp_char_buff, 128);
							submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
						}

						if (import_dialog(mat_desc_2.metalness_map, "import_metalness"))
						{
							submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS;
						}
					}
				}
				//ImGui::EndChild();

				ImGui::Spacing();
				ImGui::Spacing();

				ImGui::SeparatorText("SSS");
				{
					const char* sssOptions[] = { "No SSS", "Jimenez Gaussian" , "Jimenez Separable" , "Golubev" };
					int selected = (int)(mat_desc_2.selected_sss_model);
					if (ImGui::Combo("SSS Model", &selected, sssOptions, IM_ARRAYSIZE(sssOptions)))
					{
						submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_PRMS;
						mat_desc_2.selected_sss_model = (SSS_MODEL)selected;
					}


					ImGui::Spacing();

					switch (mat_desc_2.selected_sss_model)
					{
					case SSS_MODEL::JIMENEZ_GAUSS:
					{
						//ImGui::DragFloat("Mean Free Path Distance", &mat_desc_2.sss_model.mean_free_path_distance, 0.001f, 0.001f, 1000.0f, "%.4f");
						//if (ImGui::IsItemEdited())
						//{
						//	submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_PRMS;
						//}

						ImGui::DragFloat("Scale", &mat_desc_2.sss_model.scale, 0.001f, 0.001f, 1000.0f, "%.4f");
						if (ImGui::IsItemEdited())
						{
							submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_PRMS;
						}

						break;
					}
					case SSS_MODEL::JIMENEZ_SEPARABLE:
					{
						ImGui::DragFloat("Mean Free Path Distance", &mat_desc_2.sss_model.mean_free_path_distance, 0.001f, 0.001f, 1000.0f, "%.4f");
						if (ImGui::IsItemEdited())
						{
							submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_PRMS;
						}

						ImGui::DragFloat("Scale", &mat_desc_2.sss_model.scale, 0.001f, 0.001f, 1000.0f, "%.4f");
						if (ImGui::IsItemEdited())
						{
							submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_PRMS;
						}
						break;
					}
					case SSS_MODEL::GOLUBEV:
					{
						ImGui::InputFloat3("Subsurface Albedo", &mat_desc_2.sss_model.subsurface_albedo.x);
						if (ImGui::IsItemEdited())
						{
							submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_PRMS;
						}
						ImGui::DragFloat3("Mean Free Path Color", &mat_desc_2.sss_model.mean_free_path_color.x, 0.001f, 0.001f, 1000.0f, "%.3f");
						if (ImGui::IsItemEdited())
						{
							submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_PRMS;
						}
						ImGui::DragFloat("Mean Free Path Distance", &mat_desc_2.sss_model.mean_free_path_distance, 0.001f, 0.001f, 1000.0f, "%.4f");
						if (ImGui::IsItemEdited())
						{
							submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_PRMS;
						}
						ImGui::DragFloat("Scale", &mat_desc_2.sss_model.scale, 0.001f, 0.001f, 1000.0f, "%.4f");
						if (ImGui::IsItemEdited())
						{
							submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_PRMS;
						}
						int num_samples = mat_desc_2.sss_model.num_samples;
						ImGui::SliderInt("Num Samples", &num_samples, 4, 64);
						if (ImGui::IsItemEdited())
						{
							mat_desc_2.sss_model.num_samples = num_samples;
							submesh_info->hnd_material_cache.is_cached = UPDATE_MAT_PRMS;
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

}

