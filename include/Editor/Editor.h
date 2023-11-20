#ifndef EDITOR_H_
#define EDITOR_H_

#include "SceneHierarchy.h"
#include "EntityOutline.h"

#include <SceneGraph.h>
#include <RendererFrontend.h>
#include <vector>

class Editor {

public:
	void RenderEditor(RendererFrontend& rf);

	SceneHierarchy sceneHier;
	EntityOutline entityProps;
};

#endif