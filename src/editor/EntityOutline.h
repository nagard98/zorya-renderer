#ifndef ENTITY_PROPERTIES_H_
#define ENTITY_PROPERTIES_H_

#include "renderer/frontend/RendererFrontend.h"
#include "renderer/frontend/Material.h"

#include "reflection/Reflection.h"

class EntityOutline {

public:
	EntityOutline();
	~EntityOutline();

	void RenderEProperties(RenderableEntity& entity, SubmeshInfo* smInfo, MaterialDesc* matDesc);
	void RenderEProperties(RenderableEntity& entity, LightInfo& lightInfo);

	void RenderETransform(RenderableEntity& entity);

	static char tmpCharBuff[128];
};

#endif // !ENTITY_PROPERTIES_H_
