#ifndef EDITOR_H_
#define EDITOR_H_

#include "SceneHierarchy.h"
#include "EntityProperties.h"

#include <RendererFrontend.h>
#include <vector>

class Editor {

public:
	Editor();
	~Editor();

	void RenderEditor(const std::vector<RenderableEntity>& entities);

	SceneHierarchy sceneHier;
	EntityProperties entityProps;

};

#endif