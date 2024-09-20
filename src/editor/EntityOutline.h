#ifndef ENTITY_OUTLINE_H_
#define ENTITY_OUTLINE_H_

#include "renderer/frontend/SceneManager.h"
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

		//void render_entity_properties(Renderable_Entity& entity, Submesh_Info* submesh_info, Reflection_Base* material_desc);
		void render_entity_light_properties(Renderable_Entity& entity, Light_Info& light_info);
		void render_entity_mesh_properties(Renderable_Entity& entity);

		void render_entity_transform(Renderable_Entity& entity);

		//static ImGui::FileBrowser fileBrowserDialog;
		static ImGuiID id_dialog_open;

		static char tmp_char_buff[128];
	};

	bool render_entity_property(CB_Variable& property, void* cb_start);
	bool render_entity_property(float* field_addr, int num_components, const char* name);
	bool render_entity_property(uint32_t* field_addr, int num_components, const char* name);
	bool render_entity_property(bool* field_addr, int num_components, const char* name);

}


#endif // !ENTITY_PROPERTIES_H_
