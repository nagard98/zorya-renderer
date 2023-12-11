#ifndef ENTITY_PROPERTIES_H_
#define ENTITY_PROPERTIES_H_

#include <RendererFrontend.h>
#include "Material.h"

class EntityOutline {

public:
	EntityOutline();
	~EntityOutline();

	void RenderEProperties(RenderableEntity& entity, SubmeshInfo* smInfo, MaterialDesc* matDesc);
	void RenderEProperties(RenderableEntity& entity, LightInfo& lightInfo);

	char tmpCharBuff[128];
};

#endif // !ENTITY_PROPERTIES_H_
