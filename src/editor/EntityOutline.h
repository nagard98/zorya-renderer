#ifndef ENTITY_PROPERTIES_H_
#define ENTITY_PROPERTIES_H_

#include "renderer/frontend/RendererFrontend.h"
#include "renderer/frontend/Material.h"

#include "reflection/Reflection.h"

#include "imgui.h"
#include "imfilebrowser.h"

class EntityOutline {

public:
	EntityOutline();
	~EntityOutline();

	void RenderEProperties(RenderableEntity& entity, SubmeshInfo* smInfo, ReflectionBase* matDesc);
	void RenderEProperties(RenderableEntity& entity, LightInfo& lightInfo);

	void RenderETransform(RenderableEntity& entity);

	//static ImGui::FileBrowser fileBrowserDialog;
	static ImGuiID idDialogOpen;

	static char tmpCharBuff[128];
};

#endif // !ENTITY_PROPERTIES_H_
