#include "AssetRegistry.h"
#include "data_structures/Tree.h"
#include "renderer/frontend/Asset.h"

#include "Windows.h"
#include "Shlwapi.h"
#include <cassert>

namespace zorya
{
	Asset_Registry asset_registry;
}

zorya::Filesystem_Node::Filesystem_Node(Filesystem_Node_Type type, const char* path, const char* name) : m_type(type)
{
	switch (m_type)
	{
	case zorya::Filesystem_Node_Type::FOLDER:
	{
		size_t name_buff_size = strnlen_s(name, MAX_PATH) + 1;
		m_name = (char*)malloc(name_buff_size);
		strncpy_s(m_name, name_buff_size, name, name_buff_size);

		size_t path_buff_size = strnlen_s(path, MAX_PATH) + 1;
		m_folder_path = (char*)malloc(path_buff_size);
		strncpy_s(m_folder_path, path_buff_size, path, path_buff_size);
	}
		break;
	case zorya::Filesystem_Node_Type::FILE: 
	{
		m_asset_with_config.import_config = Asset_Import_Config::deserialize(path);//create_asset_import_config(path);
		m_asset_with_config.asset = create_asset(m_asset_with_config.import_config);

		size_t name_buff_size = strnlen_s(name, MAX_PATH) - strnlen_s(".metafile", MAX_PATH) + 1;
		m_name = (char*)malloc(name_buff_size);
		strncpy_s(m_name, name_buff_size, name, name_buff_size - 1);
	}
		break;
	default:
		break;
	}
}

zorya::Filesystem_Node::Filesystem_Node(Filesystem_Node_Type type, Asset* asset, Asset_Import_Config* import_conf, const char* name)
{
	assert(import_conf != nullptr);

	m_type = type;

	size_t name_buff_size = strnlen_s(name, MAX_PATH) + 1;
	m_name = (char*)malloc(name_buff_size);
	strncpy_s(m_name, name_buff_size, name, name_buff_size - 1);

	m_asset_with_config.import_config = import_conf;
	m_asset_with_config.asset = (asset == nullptr ? create_asset(import_conf) : asset);
}

zorya::Filesystem_Node::~Filesystem_Node()
{
	//TODO: create destructor for filesystem node
}

zorya::Filesystem_Node::Filesystem_Node(const Filesystem_Node& value_to_copy)
{
	m_type = value_to_copy.m_type;

	size_t name_buff_size = strnlen_s(value_to_copy.m_name, MAX_PATH) + 1;
	m_name = (char*)malloc(name_buff_size);
	strncpy_s(m_name, name_buff_size, value_to_copy.m_name, name_buff_size);

	switch (m_type)
	{
	case zorya::Filesystem_Node_Type::FOLDER:
	{
		size_t path_buff_size = strnlen_s(value_to_copy.m_folder_path, MAX_PATH) + 1;
		m_folder_path = (char*)malloc(path_buff_size);
		strncpy_s(m_folder_path, path_buff_size, value_to_copy.m_folder_path, path_buff_size);
	}
		break;
	case zorya::Filesystem_Node_Type::FILE:
	{
		//TODO: check if Asset copy constructor is implemented correctly
		m_asset_with_config = value_to_copy.m_asset_with_config;
	}
		break;
	default:
		break;
	}
}

zorya::Filesystem_Node::Filesystem_Node(Filesystem_Node&& value_to_move)
{
	m_type = value_to_move.m_type;

	m_name = value_to_move.m_name;
	value_to_move.m_name = nullptr;

	switch (m_type)
	{
	case zorya::Filesystem_Node_Type::FOLDER:
	{
		m_folder_path = value_to_move.m_folder_path;
		value_to_move.m_folder_path = nullptr;
	}
		break;
	case zorya::Filesystem_Node_Type::FILE:
	{
		m_asset_with_config = value_to_move.m_asset_with_config;
		value_to_move.m_asset_with_config.asset = nullptr;
		value_to_move.m_asset_with_config.import_config = nullptr;
	}
		break;
	default:
		break;
	}
}

zorya::Filesystem_Node& zorya::Filesystem_Node::operator=(Filesystem_Node& other)
{
	return other;
}


zorya::Asset_Registry::Asset_Registry()
{
	project_root = nullptr;
	current_node = nullptr;
}

HRESULT zorya::Asset_Registry::init(const char* project_root_path)
{
	WIN32_FIND_DATA found_file_info;
	HANDLE hnd_find_file = FindFirstFileA(project_root_path, &found_file_info);

	if (hnd_find_file != INVALID_HANDLE_VALUE)
	{
		if (found_file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{

			Filesystem_Node root(Filesystem_Node_Type::FOLDER, project_root_path, "Assets");
			project_root = new Node2<Filesystem_Node>(std::move(root), nullptr);
			current_node = project_root;
			recursive_build(project_root_path, project_root->children);
		}

		return S_OK;
	} else
	{
		return E_ABORT;
	}

}

zorya::Asset_With_Config* recursive_search_by_guid(Node2<zorya::Filesystem_Node>* root_node, uint64_t guid)
{
	zorya::Asset_With_Config* found_asset = nullptr;

	for (auto& node : root_node->children)
	{
		switch (node.value.m_type)
		{
		case zorya::Filesystem_Node_Type::FILE:
		{
			if (node.value.m_asset_with_config.asset->m_guid == guid) return &node.value.m_asset_with_config;
			break;
		}
		case zorya::Filesystem_Node_Type::FOLDER:
		{
			found_asset = recursive_search_by_guid(&node, guid);
			break;
		}
		default:
			break;
		}
	}

	return found_asset;
}

zorya::Asset_With_Config* zorya::Asset_Registry::get_asset_by_guid(uint64_t guid) const
{
	assert(project_root != nullptr);
	
	Asset_With_Config* found_asset = (guid != 0 ? recursive_search_by_guid(project_root, guid) : nullptr);
	if (found_asset != nullptr) found_asset->asset->load_asset(found_asset->import_config);

	return found_asset;
}

void zorya::Asset_Registry::recursive_build(const char* project_current_path, std::vector<Node2<Filesystem_Node>>& folder_nodes)
{
	WIN32_FIND_DATA found_file_info;
	size_t path_buff_size = strnlen_s(project_current_path, MAX_PATH) + 3;
	char* current_path = (char*)malloc(path_buff_size);
	strncpy_s(current_path, path_buff_size, project_current_path, path_buff_size);
	strncat_s(current_path, path_buff_size, "\\*", path_buff_size);

	HANDLE hnd_find_file; //= FindFirstFileA(current_path, &found_file_info);

	if ((hnd_find_file = FindFirstFileA(current_path, &found_file_info)) != INVALID_HANDLE_VALUE)
	{
		free(current_path);

		while (FindNextFileA(hnd_find_file, &found_file_info))
		{

			if ((found_file_info.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == 0)
			{
				path_buff_size = strnlen_s(project_current_path, MAX_PATH) + 3 + strnlen_s(found_file_info.cFileName, MAX_PATH);
				current_path = (char*)malloc(path_buff_size);
				strncpy_s(current_path, path_buff_size, project_current_path, path_buff_size);
				strncat_s(current_path, path_buff_size, "\\", path_buff_size);
				strncat_s(current_path, path_buff_size, found_file_info.cFileName, path_buff_size);

				if ((found_file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
				{
					if (strncmp(found_file_info.cFileName, "..", MAX_PATH) != 0)
					{
						//TODO: check why folder_nodes is initialized with capacity=1
						Node2<Filesystem_Node>& folder_node = folder_nodes.emplace_back(Filesystem_Node(Filesystem_Node_Type::FOLDER, current_path, found_file_info.cFileName), current_node);

						Node2<Filesystem_Node>* tmp_node = current_node;
						current_node = &folder_node;
						recursive_build(current_path, folder_node.children);
						current_node = tmp_node;
					}
				} else
				{
					char* extension = PathFindExtension(current_path);
					if (strncmp(extension, ".metafile", strnlen(".metafile", MAX_PATH)) == 0)
					{
						folder_nodes.emplace_back(Filesystem_Node(Filesystem_Node_Type::FILE, current_path, found_file_info.cFileName), current_node);
					}
				}

				free(current_path);
			}	
		}
		FindClose(hnd_find_file);

	} else
	{
		free(current_path);
	}
}
