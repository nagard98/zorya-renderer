#ifndef ASSET_H_
#define ASSET_H_

#include <vector>
#include <string>
#include <Windows.h>

namespace zorya
{
	
	enum Asset_Extension
	{
		JPG,
		PNG,
		TXT,
		NUM_SUPPORTED_EXTENSIONS,
		UNKNOWN
	};

	enum Asset_Type
	{
		TEXTURE,
		STATIC_MESH,
		MATERIAL,
		DIFFUSION_PROFILE,
		UNSUPPORTED
	};

	static const char* Asset_Type_Names[] = {
		"Texture",
		"Static_Mesh",
		"Material",
		"Diffusion Profile"
	};

	struct Asset_Import_Config
	{
		Asset_Import_Config() {};
		Asset_Import_Config(Asset_Type asset_type, const char* asset_file_path);

		static Asset_Import_Config* deserialize(const char* metafile_path);
		static int serialize(const Asset_Import_Config* asset_imp_config);

		uint64_t guid;
		char* asset_filepath;
		Asset_Type asset_type;
	};

	class Asset
	{
	public:
		Asset();
		Asset(const char* filepath);
		Asset(const Asset_Import_Config* asset_import_config);
		virtual ~Asset() {};

		uint64_t m_guid;
		std::string m_file_path;
		Asset_Type asset_type;
		bool m_is_loaded;

		virtual void load_asset(const Asset_Import_Config* asset_imp_conf) = 0;
	};

	struct Asset_With_Config
	{
		Asset_Import_Config* import_config;
		Asset* asset;
	};


	Asset_Import_Config* create_asset_import_config(const char* metafile_path); //loads existing metafile
	Asset_Import_Config* create_asset_import_config(Asset_Type asset_type, const char* referenced_asset_file_path); //creates new metafile base on referenced asset

	Asset* create_asset(const Asset_Import_Config* asset_imp_conf);

	Asset_Type get_asset_type_by_ext(const char* extension);

	//class Text_Asset : public Asset
	//{
	//public:
	//	Text_Asset(const char* filepath);
	//	void load_asset() {};
	//};
}


#endif // !ASSET_H_
