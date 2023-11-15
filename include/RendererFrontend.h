#ifndef RENDERER_FRONTEND_H_
#define RENDERER_FRONTEND_H_

#include "Model.h"
#include "BufferCache.h"
#include "ResourceCache.h"

#include <DirectXMath.h>

#include "assimp/Importer.hpp"

namespace dx = DirectX;

struct SubmeshInfo {
	SubmeshHandle_t submeshHnd;
	BufferCacheHandle_t bufferHnd;
	MaterialCacheHandle_t matHnd;
	dx::XMMATRIX worldTrans;
};

struct ViewDesc {
	std::vector<SubmeshInfo> submeshBufferPairs;
};

class RendererFrontend {
	//std::vector<SubmeshHandle> submeshHandles; //indexed by model handle
public:
	RendererFrontend();
	~RendererFrontend();

	void InitScene();

	//TODO: probably move somewhere else
	ModelHandle_t LoadModelFromFile(const std::string& filename);
	ViewDesc ComputeView();

	//TODO: Maybe should be BVH instead of linear list? Remember Frostbite presentation.
	std::vector<SubmeshInfo> sceneMeshes;
	
	std::vector<MaterialDesc> materials; //material indexed by part of submesh handle
	std::vector<Vertex> staticSceneVertexData; //mesh vertex data base indexed by part of submesh handle
	std::vector<std::uint16_t> staticSceneIndexData; //mesh index data base indexed by part of submesh handle

private:
	Assimp::Importer importer;
};


extern RendererFrontend rf;

#endif // !RENDERER_FRONTEND_H_
