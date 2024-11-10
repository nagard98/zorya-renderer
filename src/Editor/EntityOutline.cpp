#include "editor/EntityOutline.h"
#include "editor/Logger.h"
#include "Editor.h"

#include "renderer/frontend/SceneManager.h"
#include "renderer/frontend/Material.h"

#include "renderer/backend/Renderer.h"

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
			for_each_field([&](auto& refl_field, auto& field_meta)
				{
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

		//TODO: fix
		//if (ImGui::Button("Import"))
		//{
		//	file_browser.Open();
		//	Entity_Outline::id_dialog_open = id_current_dialog;
		//}

		//if (Entity_Outline::id_dialog_open == id_current_dialog && file_browser.HasSelected())
		//{
		//	std::wstring importFilepath = file_browser.GetSelected().wstring();
		//	wcsncpy_s(field_addr, 128, importFilepath.c_str(), wcsnlen_s(importFilepath.c_str(), MAX_PATH));
		//	file_browser.ClearSelected();
		//	is_editing_complete = true;
		//}

		ImGui::PopID();

		ImGui::Spacing();

		return is_editing_complete;
	}

	bool render_entity_property(float* field_addr, int num_components, const char* name)
	{
		switch (num_components)
		{
		case(1):
		{
			ImGui::DragFloat(name, field_addr, 0.01f, 0.001f, 0.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			break;
		}
		case(2):
		{
			ImGui::DragFloat2(name, field_addr, 0.01f, 0.001f, 0.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			break;
		}
		case(3):
		{
			ImGui::DragFloat3(name, field_addr, 0.01f, 0.001f, 0.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			break;
		}
		case(4):
		{
			ImGui::DragFloat4(name, field_addr, 0.01f, 0.001f, 0.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			break;
		}
		default:
			break;
		}

		return ImGui::IsItemEdited();
	}

	bool render_entity_property(uint32_t* field_addr, int num_components, const char* name)
	{
		switch (num_components)
		{
		case(1):
		{
			ImGui::DragInt(name, (int*)field_addr, 0, 0, 1, "%d", ImGuiSliderFlags_AlwaysClamp);
			break;
		}
		case(2):
		{
			ImGui::DragInt2(name, (int*)field_addr, 0, 0, 1, "%d", ImGuiSliderFlags_AlwaysClamp);
			break;
		}
		case(3):
		{
			ImGui::DragInt3(name, (int*)field_addr, 0, 0, 1, "%d", ImGuiSliderFlags_AlwaysClamp);
			break;
		}
		case(4):
		{
			ImGui::DragInt4(name, (int*)field_addr, 0, 0, 1, "%d", ImGuiSliderFlags_AlwaysClamp);
			break;
		}
		default:
			break;
		}

		return ImGui::IsItemEdited();
	}

	bool render_entity_property(bool* field_addr, int num_components, const char* name)
	{
		ImGui::Checkbox(name, field_addr);
		return ImGui::IsItemEdited();
	}


	bool render_entity_property(CB_Variable& property, void* cb_start)
	{
		unsigned char* fieldAddr = static_cast<unsigned char*>(cb_start) + property.offset_in_bytes;

		switch (property.description.variable_type)
		{
		case (VAR_REFL_TYPE::FLOAT):
		{
			return render_entity_property((float*)fieldAddr, property.description.columns, property.name);
			break;
		}
		case (VAR_REFL_TYPE::UINT32):
		{
			return render_entity_property((uint32_t*)fieldAddr, property.description.columns, property.name);
			break;
		}
		case (VAR_REFL_TYPE::BOOL):
		{
			return render_entity_property((bool*)fieldAddr, property.description.columns, property.name);
			break;
		}
		default:
		{
			return false;
			break;
		}
		}
	}

	template <typename T>
	struct RenderProperty
	{
		static void render(T& stru)
		{
			OutputDebugString("rendering struct\n");
		}
	};


	void Entity_Outline::render_entity_light_properties(Renderable_Entity& entity, Light_Info& light_info)
	{
		render_entity_transform(entity);

		ImGui::SeparatorText("Light Parameters");
		{
			switch (light_info.tag)
			{

			case Light_Type::DIRECTIONAL:
				for_each_field([](auto& struct_addr, auto& field_meta)
					{
						render_entity_property(field_meta.type.cast_to((void*)GET_FIELD_ADDRESS(struct_addr, field_meta.offset)), field_meta.name);
					}, light_info.dir_light);
				break;

			case Light_Type::SPOT:
				for_each_field([](auto& struct_addr, auto& field_meta)
					{
						render_entity_property(field_meta.type.cast_to((void*)GET_FIELD_ADDRESS(struct_addr, field_meta.offset)), field_meta.name);
					}, light_info.spot_light);
				break;

			case Light_Type::POINT:
				for_each_field([](auto& struct_addr, auto& field_meta)
					{
						render_entity_property(field_meta.type.cast_to((void*)GET_FIELD_ADDRESS(struct_addr, field_meta.offset)), field_meta.name);
					}, light_info.point_light);
				break;

			case Light_Type::SKYLIGHT:
			{
				Texture2D* env_tex = light_info.sky_light.environment_texture;
				u64 null_id = 0;
				ImGui::InputScalar("Environment Texture", ImGuiDataType_U64, env_tex == nullptr ? &null_id : &env_tex->m_guid);
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(Asset_Type_Names[Asset_Type::TEXTURE]))
					{
						auto payload_data = (Asset_With_Config*)payload->Data;
						Texture2D* dropped_tex = static_cast<Texture2D*>(payload_data->asset);
						const Texture_Import_Config* dropped_imp_conf = static_cast<Texture_Import_Config*>(payload_data->import_config);
						dropped_tex->load_asset(dropped_imp_conf);
						light_info.sky_light.environment_texture = dropped_tex;
						Render_SRV_Handle environment_map_srv{ 0 };
						rhi.load_texture2(dropped_tex, dropped_imp_conf, &environment_map_srv);
						renderer.build_ibl_data(light_info.sky_light, environment_map_srv);
					}

					ImGui::EndDragDropTarget();
				}

				break;
			}

			default : 
				zassert(false);
			
			}

		}

	}

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


	void Entity_Outline::render_entity_mesh_properties(Renderable_Entity& entity/*, Submesh_Render_Data* submesh_info*/)
	{
		render_entity_transform(entity);
		
		//TODO: better check if entity has mesh; probably move check to callee of this function
		if (entity.submesh_desc != nullptr)
		{
			Material_Cache_Handle_t hnd_material = entity.submesh_desc->hnd_material_cache;
			Material& material = resource_cache.m_material_cache.at(hnd_material.index);

			bool is_edited = false;

			ImGui::SeparatorText("Rendering");
			{
				int mat_index = entity.submesh_desc->hnd_material_cache.index;
				ImGui::InputInt("Material", &mat_index);
				entity.submesh_desc->hnd_material_cache.index = mat_index;
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(Asset_Type_Names[Asset_Type::MATERIAL]))
					{
						auto payload_data = (Asset_With_Config*)payload->Data;
						Material* dropped_mat = static_cast<Material*>(payload_data->asset);
						Asset_Import_Config* dropped_imp_conf = payload_data->import_config;
						dropped_mat->load_asset(dropped_imp_conf);
						entity.submesh_desc->hnd_material_cache = dropped_mat->material_hnd;
						is_edited |= true;
					}

					ImGui::EndDragDropTarget();
				}
			}

		}

	}

}

