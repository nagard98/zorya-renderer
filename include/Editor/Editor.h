#ifndef EDITOR_H_
#define EDITOR_H_

#include "SceneHierarchy.h"
#include "EntityOutline.h"

#include <SceneGraph.h>
#include <RendererFrontend.h>
#include <vector>

#include "imgui.h"
#include <d3d11_1.h>

class Editor {

public:
	Editor();

	void RenderEditor(RendererFrontend& rf, const ID3D11ShaderResourceView* rtSRV);

	SceneHierarchy sceneHier;
	EntityOutline entityProps;
	bool firstLoop;
	ImGuiID sceneId;

	char textBuff[128];
};

#endif