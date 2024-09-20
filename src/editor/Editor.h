#ifndef EDITOR_H_
#define EDITOR_H_

#include "editor/SceneHierarchy.h"
#include "editor/EntityOutline.h"
#include "editor/ContentBrowser.h"
#include "editor/AssetInspector.h"

#include "renderer/frontend/SceneGraph.h"
#include "renderer/frontend/SceneManager.h"

#include "imgui.h"
#include "imfilebrowser.h"

#include <vector>
#include <string>
#include <d3d11_1.h>

namespace zorya
{
	extern ImGui::FileBrowser file_browser;

	struct Type_Filter_Index
	{
		enum
		{
			PNG = 0,
			JPG = 1,
			JPEG = 2
		};
	};

	class Editor
	{

	public:
		Editor();

		HRESULT init(HWND window_hnd, Render_Hardware_Interface& rhi);
		void shutdown();
		
		void setup_dockspace(const ImGuiID& dockspace_ID);

		void new_frame();
		void render_editor(Scene_Manager& rf, Camera& editorCam, const ID3D11ShaderResourceView* rtSRV);

		bool m_file_browser_open = false;

		Scene_Hierarchy m_scene_hierarchy;
		Entity_Outline m_entity_properties;
		Asset_Inspector m_asset_inspector;
		Content_Browser m_content_browser;

		bool m_first_loop;
		ImGuiID m_scene_id;

		bool m_is_scene_clicked;
		char m_text_buffer[128];
	};

	extern Editor editor;
}

#endif