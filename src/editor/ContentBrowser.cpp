#include "ContentBrowser.h"
#include "editor/AssetRegistry.h"
#include "editor/Editor.h"

#include <vector>
#include "imgui.h"
#include "renderer/backend/rhi/RenderHardwareInterface.h"
#include "renderer/backend/ResourceCache.h"
#include "renderer/frontend/Material.h"
#include "renderer/frontend/DiffusionProfile.h"

namespace zorya
{
	void Content_Browser::recursive_render_asset_tree(std::vector<Node2<Filesystem_Node>>& nodes) {
		for (auto& node : nodes)
		{
			if (node.value.m_type == Filesystem_Node_Type::FOLDER)
			{
				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
				if (asset_registry.current_node == &node)
				{
					flags |= ImGuiTreeNodeFlags_Selected;
				}

				bool is_node_open = ImGui::TreeNodeEx(node.value.m_name, flags);
				if (ImGui::IsItemClicked()) asset_registry.current_node = &node;
				
				if (is_node_open)
				{
					recursive_render_asset_tree(node.children);
					ImGui::TreePop();
				}
			}
		}
	}
}

static std::string create_file(const char* folder_path, const char* filename, const char* extension)
{
	//strncat_s(folder_path, MAX_PATH, "\\", MAX_PATH);
	//int index_last_char_base_path = strnlen(folder_path, MAX_PATH);

	int file_creation_attempts = 0;
	char tentative_filename[MAX_PATH];
	ZeroMemory(tentative_filename, MAX_PATH);

	FILE* file, * metafile = nullptr;
	bool is_found_free_filename = false;
	std::string final_filepath;

	while (!is_found_free_filename)
	{
		sprintf(tentative_filename, "%s\\%s%d.%s%s", folder_path, filename, file_creation_attempts, extension, "\0");

		//material_filepath[index_last_char_base_path] = 0;
		//strncat_s(material_filepath, tentative_filename, MAX_PATH);

		file = fopen(tentative_filename, "r");

		if (file != nullptr)
		{
			fclose(file);
			file_creation_attempts += 1;
		} else
		{
			file = fopen(tentative_filename, "wb");
			final_filepath = std::string(tentative_filename);

			//strncat_s(material_filepath, ".metafile", MAX_PATH);
			//metafile = fopen(material_filepath, "wb");

			is_found_free_filename = true;
		}
	}

	if (file != nullptr) fclose(file);
	if (metafile != nullptr) fclose(metafile);

	return final_filepath;
}

void zorya::Content_Browser::init()
{
	selected_asset = nullptr;

	ID3D11ShaderResourceView* folder_tex = nullptr;
	RHI_RESULT res = rhi.load_texture(L".\\assets\\folder.png", &folder_tex, true);
	if (res != S_OK)
	{
		folder_tex = nullptr;
		m_hnd_folder_tex.index = 0;
	}else
	{
		m_hnd_folder_tex = rhi.add_srv(std::move(folder_tex));
	}
	
	ID3D11ShaderResourceView* file_tex = nullptr;
	res = rhi.load_texture(L".\\assets\\free-file-icons\\48px\\_blank.png", &file_tex, true);
	if (res != S_OK)
	{
		file_tex = nullptr;
		m_hnd_file_tex.index = 0;
	} else
	{
		m_hnd_file_tex = rhi.add_srv(std::move(file_tex));
	}
	//TODO: destroy texture on program close
}

void zorya::Content_Browser::render(Asset_Registry& asset_registry)
{
	float region_width = ImGui::GetWindowContentRegionMax().x;
	float ratio_width = 0.25f;

	static int selected_item = -1;

	ImGui::BeginChild("Content_Tree", ImVec2(region_width * ratio_width, 0.0f), true);
	{
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow;
		if (asset_registry.current_node == asset_registry.project_root)
		{
			flags |= ImGuiTreeNodeFlags_Selected;
		}

		bool is_node_open = ImGui::TreeNodeEx(asset_registry.project_root->value.m_name, flags);
		if (ImGui::IsItemClicked())
		{
			asset_registry.current_node = asset_registry.project_root;
			selected_item = -1;
		}

		if (is_node_open)
		{
			recursive_render_asset_tree(asset_registry.project_root->children);
			ImGui::TreePop();
		}
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("Content_Explorer", ImVec2(region_width * (1.0f - ratio_width), 0.0f));
	{
		ImGuiStyle& style = ImGui::GetStyle();
		int num_assets = asset_registry.current_node->children.size();
		ImVec2 button_sz(80, 90);
		int window_vis_region = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

		for (int i = 0; i < num_assets; i++)
		{
			ImGui::PushID(i);

			auto& node = asset_registry.current_node->children[i];
			Filesystem_Node_Type node_type = node.value.m_type;
			char label[MAX_PATH];

			ImGui::BeginGroup();

			switch (node_type)
			{
			case(Filesystem_Node_Type::FOLDER):
			{
				auto pos = ImGui::GetCursorPos();

				strncpy_s(label, "folder", MAX_PATH);
				if (ImGui::Selectable("##title", selected_item == i, 0, button_sz))
				{
					selected_item = i;
					selected_asset = nullptr;
				}

				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
				{
					asset_registry.current_node = &node;
					i = num_assets + 1;
					selected_item = -1;
				}

				pos.x += 5.0f;
				pos.y += 5.0f;

				ImGui::SetCursorPos(pos);
				ImGui::Image(rhi.get_srv_pointer(m_hnd_folder_tex), ImVec2(65, 65));

				break;
			}
			case(Filesystem_Node_Type::FILE):
			{
				strncpy_s(label, "asset", MAX_PATH);

				auto pos = ImGui::GetCursorPos();
				if (ImGui::Selectable("##title", selected_item == i, 0, button_sz))
				{
					assert(node.value.m_asset_with_config.asset != nullptr);
					selected_item = i;
					
					selected_asset = &node.value.m_asset_with_config;
					selected_asset->asset->load_asset(selected_asset->import_config);
	
					ImGui::SetWindowFocus("Asset Inspector");
				}

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload(Asset_Type_Names[node.value.m_asset_with_config.import_config->asset_type], &node.value.m_asset_with_config, sizeof(node.value.m_asset_with_config));
					ImGui::EndDragDropSource();
				}
				
				pos.x += 5.0f;
				pos.y += 5.0f;

				ImGui::SetCursorPos(pos);
				ImGui::Image(rhi.get_srv_pointer(m_hnd_file_tex), ImVec2(65, 65));

				break;
			}
			}
			
			int last_but_x = ImGui::GetItemRectMax().x;
			ImGui::Text("%.12s", node.value.m_name);
			ImGui::EndGroup();

			if (ImGui::BeginItemTooltip())
			{
				ImGui::Text("%s", node.value.m_name);
				ImGui::EndTooltip();
			}

			if (i + 1 < num_assets && last_but_x + style.ItemSpacing.x + button_sz.x < window_vis_region)
			{
				ImGui::SameLine();
				auto pos = ImGui::GetCursorPos();
				pos.x += style.ItemSpacing.x;
				ImGui::SetCursorPos(pos);
			}

			ImGui::PopID();
		}
	}

	ImGui::EndChild();
	
	if (ImGui::BeginPopupContextItem("content_browser_popup"))
	{
		if (ImGui::BeginMenu("Create"))
		{
			Filesystem_Node& node = asset_registry.current_node->value;
			assert(node.m_type == Filesystem_Node_Type::FOLDER);

			int index_last_char_base_path = strnlen(asset_registry.current_node->value.m_folder_path, MAX_PATH) + 1;

			if (ImGui::MenuItem("Material"))
			{
				std::string final_asset_filepath = create_file(asset_registry.current_node->value.m_folder_path, "Material", "mat");
				create_file(asset_registry.current_node->value.m_folder_path, "Material", "mat.metafile");

				//TODO: dont use global allocator
				Asset_Import_Config* asset_imp_config = create_asset_import_config(Asset_Type::MATERIAL, final_asset_filepath.c_str());
				Material* asset = dynamic_cast<Material*>(create_asset(asset_imp_config));

				int res = asset->serialize();

				if (res == 0 && asset_imp_config != nullptr) OutputDebugString("Failed serialization of material\n");
				else
				{
					asset_registry.current_node->children.emplace_back(Filesystem_Node(Filesystem_Node_Type::FILE, asset, asset_imp_config, &asset->m_file_path.c_str()[index_last_char_base_path]/*tentative_filename*/), asset_registry.current_node);
				}
			}
			
			if (ImGui::MenuItem("Diffusion Profile"))
			{
				std::string final_asset_filepath = create_file(asset_registry.current_node->value.m_folder_path, "DiffusionProfile", "dprofile");
				create_file(asset_registry.current_node->value.m_folder_path, "DiffusionProfile", "dprofile.metafile");

				//TODO: dont use global allocator
				Asset_Import_Config* asset_imp_config = create_asset_import_config(Asset_Type::DIFFUSION_PROFILE, final_asset_filepath.c_str());
				Diffusion_Profile* asset = dynamic_cast<Diffusion_Profile*>(create_asset(asset_imp_config));

				int res = asset->serialize();

				if (res == 0 && asset_imp_config != nullptr) OutputDebugString("Failed serialization of diffusion profile\n");
				else
				{
					asset_registry.current_node->children.emplace_back(Filesystem_Node(Filesystem_Node_Type::FILE, asset, asset_imp_config, &asset->m_file_path.c_str()[index_last_char_base_path]/*tentative_filename*/), asset_registry.current_node);
				}
			}

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Import"))
		{
			if (ImGui::MenuItem("Texture"))
			{
				file_browser.SetTypeFilters( {".png", ".jpg", ".jpeg", ".hdr"});
				file_browser.Open();
			}

			if (ImGui::MenuItem("3D Model"))
			{
				file_browser.SetTypeFilters( {".fbx", ".obj", ".gltf"} );
				file_browser.Open();
			}
			ImGui::EndMenu();
		}
		ImGui::EndPopup();
	}


}
