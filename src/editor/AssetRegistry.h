#ifndef ASSET_REGISTRY_H_
#define ASSET_REGISTRY_H_

#include <vector>
#include <memory>
#include <string>

#include "data_structures/Tree.h"
#include "renderer/frontend/Asset.h"

#include <Windows.h>

namespace zorya
{
	enum class Filesystem_Node_Type
	{
		FOLDER,
		FILE
	};

	struct Filesystem_Node
	{
		Filesystem_Node(Filesystem_Node_Type type, const char* path, const char* name);
		Filesystem_Node(Filesystem_Node_Type type, Asset* asset, Asset_Import_Config* import_conf, const char* name);
		~Filesystem_Node();

		Filesystem_Node(const Filesystem_Node& value_to_copy);
		Filesystem_Node(Filesystem_Node&& value_to_move);

		Filesystem_Node& operator=(Filesystem_Node& other);

		Filesystem_Node_Type m_type;
		char* m_name;
		
		union
		{
			char* m_folder_path;
			Asset_With_Config m_asset_with_config;
			//Asset* m_asset;
		};
	};

	class Asset_Registry
	{
	public:
		Asset_Registry();
		//Asset_Registry(const char* project_root_path);
		
		HRESULT init(const char* project_root_path);

		Asset_With_Config* get_asset_by_guid(uint64_t guid) const;

		Node2<Filesystem_Node>* project_root;
		Node2<Filesystem_Node>* current_node;

	private:
		void recursive_build(const char* project_root_path, std::vector<Node2<Filesystem_Node>>& folder_nodes);

	};

	extern Asset_Registry asset_registry;

}



#endif