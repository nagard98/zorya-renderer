#ifndef TEXTURE_2D_H_
#define TEXTURE_2D_H_

#include "Asset.h"

#include <cstdint>

namespace zorya
{

	struct Texture_Import_Config : public Asset_Import_Config
	{
		Texture_Import_Config() {};
		Texture_Import_Config(const char* asset_file_path);

		static Texture_Import_Config* deserialize(const char* metafile_path);
		static int serialize(Texture_Import_Config* tex_imp_conf);

		uint32_t max_width;
		uint32_t max_height;
		uint8_t channels;
		bool is_normal_map = false;
	};


	class Texture2D : public Asset
	{
	public:
		static Texture2D* create(const Texture_Import_Config* tex_imp_conf);
		Texture2D(const char* filepath);
		Texture2D(const Texture_Import_Config* tex_import_config);

		void load_asset(const Asset_Import_Config* tex_imp_config) override;
		
		static int load_asset_info(Texture_Import_Config* tex_imp_config);

		void update_asset(const Texture_Import_Config* tex_imp_config);

		unsigned char* m_data;

	};
}

#endif // !TEXTURE_2D_H_
