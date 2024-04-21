#ifndef ENTITY_PROPERTIES_H_
#define ENTITY_PROPERTIES_H_

#include "renderer/frontend/RendererFrontend.h"
#include "renderer/frontend/Material.h"

#include "reflection/Reflection.h"

#include "imgui.h"
#include "imfilebrowser.h"

namespace zorya
{
	class Entity_Outline
	{

	public:
		Entity_Outline();
		~Entity_Outline();

		void render_entity_properties(Renderable_Entity& entity, Submesh_Info* submesh_info, Reflection_Base* material_desc);
		void render_entity_properties(Renderable_Entity& entity, Light_Info& light_info);

		void render_entity_transform(Renderable_Entity& entity);

		//static ImGui::FileBrowser fileBrowserDialog;
		static ImGuiID id_dialog_open;

		static char tmp_char_buff[128];
	};

}


#endif // !ENTITY_PROPERTIES_H_
