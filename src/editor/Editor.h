#ifndef EDITOR_H_
#define EDITOR_H_

#include "editor/SceneHierarchy.h"
#include "editor/EntityOutline.h"

#include "renderer/frontend/SceneGraph.h"
#include "renderer/frontend/RendererFrontend.h"

#include "imgui.h"
#include "imfilebrowser.h"

#include <vector>
#include <d3d11_1.h>

namespace zorya
{
	static ImGui::FileBrowser file_browser;

	class Editor
	{

	public:
		Editor();
		void init(const ImGuiID& dockspace_ID);

		void render_editor(Renderer_Frontend& rf, Camera& editorCam, const ID3D11ShaderResourceView* rtSRV);


		bool m_file_browser_open = false;

		Scene_Hierarchy m_scene_hierarchy;
		Entity_Outline m_entity_properties;
		bool m_first_loop;
		ImGuiID m_scene_id;

		bool m_is_scene_clicked;
		char m_text_buffer[128];
	};

}

#endif