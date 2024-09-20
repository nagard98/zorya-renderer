#ifndef CONTENT_BROWSER_H_
#define CONTENT_BROWSER_H_

#include "editor/AssetRegistry.h"
#include "renderer/backend/rhi/RenderHardwareInterface.h"

namespace zorya
{
	class Content_Browser
	{
	public:
		void init();
		void render(Asset_Registry& asset_registry);
		
		Asset_With_Config* selected_asset;

	private:
		void recursive_render_asset_tree(std::vector<Node2<Filesystem_Node>>& nodes);

		//Shader_Texture2D* m_folder_tex;
		Render_SRV_Handle m_hnd_folder_tex;
		//Shader_Texture2D* m_file_tex;
		Render_SRV_Handle m_hnd_file_tex;

	};
}


#endif