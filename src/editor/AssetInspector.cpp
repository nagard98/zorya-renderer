#include "AssetInspector.h"
#include "EntityOutline.h"

#include "renderer/frontend/Asset.h"
#include "renderer/frontend/Material.h"
#include "renderer/backend/JimenezSeparable.h"
#include "renderer/frontend/DiffusionProfile.h"
#include "renderer/backend/rhi/RenderHardwareInterface.h"

#include "imgui.h"

#include <random>

void zorya::Asset_Inspector::render(Scene_Manager& scene_manager, Asset_With_Config* selected_asset)
{
	if (selected_asset == nullptr)
	{
		ImGui::Text("%s", "Select asset in the content browser");
		return;
	}
	
	switch (selected_asset->import_config->asset_type)
	{
	case zorya::TEXTURE:
	{
		Texture2D* tex_asset = static_cast<Texture2D*>(selected_asset->asset);
		Texture_Import_Config* tex_imp_conf = static_cast<Texture_Import_Config*>(selected_asset->import_config);

		ImGui::SeparatorText("Texture");
		{
			render_entity_property(&tex_imp_conf->is_normal_map, 1, "is_normal_map");
		}

		if (ImGui::Button("Save"))
		{
			Texture_Import_Config::serialize(tex_imp_conf);
			tex_asset->update_asset(tex_imp_conf);
		}

		break;
	}
	case zorya::STATIC_MESH:
	{
		Model3D* model = static_cast<Model3D*>(selected_asset->asset);
		
		ImGui::SeparatorText("Model");
		{
			int mat_index = model->global_material.index;
			ImGui::InputInt("global_material", &mat_index, 1, 100, ImGuiInputTextFlags_ReadOnly);
		}
		break;
	}
	case zorya::MATERIAL:
	{
		bool is_edited = false;
		Material& material = *static_cast<Material*>(selected_asset->asset);

		ImGui::SeparatorText("Material");
		{
			ImGui::Combo("Shading Model", (int*)&material.shading_model, shading_model_names);

			std::vector<Shader_Resource*> mat_resources = material.get_material_editor_resources();
			Constant_Buffer_Data* mat_prms = nullptr;

			for (Shader_Resource* resource : mat_resources)
			{
				switch (resource->m_type)
				{

				case (Resource_Type::ZRY_CBUFF):
				{
					Constant_Buffer_Data* cbuff_data = resource->m_parameters;

					if (strcmp(resource->m_name, "_matPrms") == 0) mat_prms = cbuff_data;

					for (int i = 0; i < cbuff_data->num_variables; i++)
					{
						CB_Variable& var = cbuff_data->variables[i];
						if (var.name[0] == '_')
						{
							is_edited |= render_entity_property(var, cbuff_data->cb_start);
						}
					}

					break;
				}

				ImGui::Spacing();

				case (Resource_Type::ZRY_TEX):
				{
					ImGui::BeginGroup();
					ImGui::ImageButton(resource->m_name, rhi.get_srv_pointer(resource->m_hnd_gpu_srv), ImVec2(25, 25));
					ImGui::SameLine();
					ImGui::Text("%s", resource->m_name);
					ImGui::EndGroup();

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(Asset_Type_Names[Asset_Type::TEXTURE]))
						{
							auto payload_data = (Asset_With_Config*)payload->Data;
							Texture2D* tex_asset = static_cast<Texture2D*>(payload_data->asset);
							Texture_Import_Config* tex_imp_conf = static_cast<Texture_Import_Config*>(payload_data->import_config);
							tex_asset->load_asset(tex_imp_conf);
							//Shader_Texture2D* res = &(*rhi.get_srv_pointer(resource->m_hnd_gpu_srv));
							rhi.load_texture2(tex_asset, tex_imp_conf, &resource->m_hnd_gpu_srv);
							resource->m_texture = tex_asset;
							is_edited |= true;
						}

						ImGui::EndDragDropTarget();
					}

					break;
				}

				default:
					break;
				}
			}
			
			ImGui::Spacing();

			if (material.shading_model == Shading_Model::SUBSURFACE_GOLUBEV || material.shading_model == Shading_Model::SUBSURFACE_JIMENEZ_GAUSS || material.shading_model == Shading_Model::SUBSURFACE_JIMENEZ_SEPARABLE)
			{
				Guid dprofile_guid = material.diff_prof_hnd.index == 0 ? 0 : resource_cache.m_diffusion_profiles.at(material.diff_prof_hnd.index).m_guid;
				ImGui::InputScalar("Diffusion Profile", ImGuiDataType_U64, &dprofile_guid);
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(Asset_Type_Names[(u8)Asset_Type::DIFFUSION_PROFILE]))
					{
						auto data = static_cast<Asset_With_Config*>(payload->Data);
						Diffusion_Profile* profile = static_cast<Diffusion_Profile*>(data->asset);
						material.diff_prof_hnd = profile->hnd;
						u32 index = scene_manager.add_diffusion_profile(material.diff_prof_hnd);

						set_constant_buff_var(mat_prms, "_sssModelArrIndex", &index, sizeof(index));
						set_constant_buff_var(mat_prms, "_sssModelId", &profile->model_id, sizeof(profile->model_id));
					}

					ImGui::EndDragDropTarget();
				}
			}

		}

		if (is_edited) update_standard_internals(material);

		if (ImGui::Button("Save"))
		{
			material.serialize();
		}

		break;
	}
	case zorya::DIFFUSION_PROFILE:
	{
		Diffusion_Profile* profile = static_cast<Diffusion_Profile*>(selected_asset->asset);
		auto& sss_params = profile->sss_params;

		ImGui::SeparatorText("Diffusion Profile");
		{
			ImGui::Combo("Model", (int*)&profile->model_id, sss_model_names[0]);

			switch (profile->model_id)
			{
			case zorya::SSS_MODEL::NONE:
				break;
			case zorya::SSS_MODEL::JIMENEZ_GAUSS:
			{
				bool is_edited = false;
				is_edited |= render_entity_property(&sss_params.mean_free_path_dist, 1, "Correction Factor (beta)");
				is_edited |= render_entity_property(&sss_params.scale, 1, "SSS Level (alfa)");
				is_edited |= render_entity_property(&sss_params.subsurface_albedo.w, 1, "Pixel Size");
				break;
			}
			case zorya::SSS_MODEL::JIMENEZ_SEPARABLE:
			{
				bool is_edited = false;
				is_edited |= render_entity_property(&sss_params.mean_free_path_dist, 1, "Kernel Width");
				is_edited |= render_entity_property(&sss_params.scale, 1, "Scale");

				if (is_edited)
				{
					auto samples = separable_sss_calculate_kernel();
					memcpy(sss_params.jimenez_samples_sss, samples.data(), sizeof(float4)* samples.size());
				}

				break;
			}
			case zorya::SSS_MODEL::GOLUBEV:
			{
				bool is_edited = false;

				is_edited |= render_entity_property(&sss_params.subsurface_albedo.x, sizeof(sss_params.subsurface_albedo) / sizeof(sss_params.subsurface_albedo.x), "Subsurface Albedo");
				is_edited |= render_entity_property(&sss_params.mean_free_path_color.x, sizeof(sss_params.mean_free_path_color) / sizeof(sss_params.mean_free_path_color.x), "Mean Free Path Color");
				is_edited |= render_entity_property(&sss_params.mean_free_path_dist, 1, "Mean Free Path Dist");
				is_edited |= render_entity_property(&sss_params.num_samples, 1, "Number Samples");
				is_edited |= render_entity_property(&sss_params.scale, 1, "Scale");

				if (is_edited)
				{
					srand(42);

					//std::random_device rd;
					std::mt19937 gen(6.0f);
					std::uniform_real_distribution<> dis(0, 1.0);//uniform distribution between 0 and 1

					for (int i = 0; i < 64; i++)
					{
						profile->sss_params.samples[i] = dis(gen);
					}

					//int to_next_float4 = 4 - (m->sss_prms.num_samples % 4);
					std::sort(profile->sss_params.samples, profile->sss_params.samples + (profile->sss_params.num_samples)); //(int)(max(0.0f, m->sssPrms.scale * 4.0f)));
					std::sort(profile->sss_params.samples + 32, profile->sss_params.samples + 32 + profile->sss_params.num_supersamples);
				}

				break;
			}
			default:
				break;
			}
		}
		break;

		if (ImGui::Button("Save"))
		{
			profile->serialize();
		}
	}
	default:
		break;
	}
	
}