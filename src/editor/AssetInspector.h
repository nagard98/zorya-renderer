#ifndef ASSET_INSPECTOR_H_
#define ASSET_INSPECTOR_H_

#include "renderer/frontend/Asset.h"
#include "renderer/frontend/SceneManager.h"

namespace zorya
{

	class Asset_Inspector
	{
	public:
		void render(Scene_Manager& scene_manager, Asset_With_Config* selected_asset);
	};

}

#endif