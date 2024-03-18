#ifndef EDITOR_H_
#define EDITOR_H_

#include "editor/SceneHierarchy.h"
#include "editor/EntityOutline.h"

#include "renderer/frontend/SceneGraph.h"
#include "renderer/frontend/RendererFrontend.h"

#include <vector>

#include "imgui.h"
#include <d3d11_1.h>

class Editor {

public:
	Editor();
	void Init(const ImGuiID& dockspaceID);

	void RenderEditor(RendererFrontend& rf, Camera& editorCam, const ID3D11ShaderResourceView* rtSRV);

	SceneHierarchy sceneHier;
	EntityOutline entityProps;
	bool firstLoop;
	ImGuiID sceneId;

	bool isSceneClicked;
	char textBuff[128];
};

#endif