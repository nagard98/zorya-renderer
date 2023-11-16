#ifndef RENDERER_FRONTEND_H_
#define RENDERER_FRONTEND_H_

#include "Model.h"
#include "BufferCache.h"
#include "ResourceCache.h"
#include "Camera.h"

#include <DirectXMath.h>

#include "assimp/Importer.hpp"

#include <string>

namespace dx = DirectX;

struct RenderableEntity {
	std::uint32_t ID;
	ModelHandle_t modelHnd;
	std::string entityName;
	dx::XMMATRIX worldTransf;
	//Bound aabb;
};

struct SubmeshInfo {
	SubmeshHandle_t submeshHnd;
	BufferCacheHandle_t bufferHnd;
	MaterialCacheHandle_t matHnd;
	dx::XMMATRIX localWorldTransf;
};

struct ViewDesc {
	std::vector<SubmeshInfo> submeshBufferPairs;
	const Camera cam;
};

class RendererFrontend {
	//std::vector<SubmeshHandle> submeshHandles; //indexed by model handle
public:
	RendererFrontend();
	~RendererFrontend();

	void InitScene();

	//TODO: probably move somewhere else
	ModelHandle_t LoadModelFromFile(const std::string& filename);
	ViewDesc ComputeView(const Camera& cam);

	std::vector<RenderableEntity> rdEntities;

	//TODO: Maybe should be BVH instead of linear list? Remember Frostbite presentation.
	std::vector<SubmeshInfo> sceneMeshes;  //scene meshes indexed by model handle
	
	std::vector<MaterialDesc> materials; //material indexed by part of submesh handle
	std::vector<Vertex> staticSceneVertexData; //mesh vertex data base indexed by part of submesh handle
	std::vector<std::uint16_t> staticSceneIndexData; //mesh index data base indexed by part of submesh handle

private:
	Assimp::Importer importer;
};


extern RendererFrontend rf;

#endif // !RENDERER_FRONTEND_H_
