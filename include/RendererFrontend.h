#ifndef RENDERER_FRONTEND_H_
#define RENDERER_FRONTEND_H_

#include "Model.h"
#include "BufferCache.h"
#include "ResourceCache.h"
#include "Camera.h"
#include "SceneGraph.h"

#include <DirectXMath.h>

#include "assimp/scene.h"
#include "assimp/Importer.hpp"

#include <string>

namespace dx = DirectX;

//struct RenderableEntityHandle_t {
//	std::uint32_t index;
//	std::uint32_t numChildren;
//	std::uint8_t isValid;
//};

struct RenderableEntity {
	bool operator==(const RenderableEntity& r) {
		return ID == r.ID;
	}

	std::uint32_t ID;
	SubmeshHandle_t submeshHnd;
	std::string entityName;
	dx::XMMATRIX localWorldTransf;
	//Bound aabb;
};

struct SubmeshInfo {
	SubmeshHandle_t submeshHnd;
	BufferCacheHandle_t bufferHnd;
	MaterialCacheHandle_t matHnd;
	dx::XMMATRIX finalWorldTransf;
};

struct ViewDesc {
	std::vector<SubmeshInfo> submeshesInfo;
	const Camera cam;
};

class RendererFrontend {
	//std::vector<SubmeshHandle> submeshHandles; //indexed by model handle
public:
	RendererFrontend();
	~RendererFrontend();

	void InitScene();

	//TODO: probably move somewhere else
	RenderableEntity LoadModelFromFile(const std::string& filename, bool forceFlattenScene = false);
	ViewDesc ComputeView(const Camera& cam);

	SceneGraph<RenderableEntity> sceneGraph;

	//TODO: Maybe should be BVH instead of linear list? Remember Frostbite presentation.
	std::vector<SubmeshInfo> sceneMeshes;  //scene meshes indexed by model handle
	
	std::vector<MaterialDesc> materials; //material indexed by part of submesh handle
	std::vector<Vertex> staticSceneVertexData; //mesh vertex data base indexed by part of submesh handle
	std::vector<std::uint16_t> staticSceneIndexData; //mesh index data base indexed by part of submesh handle

private:
	void LoadNodeChildren(const aiScene* scene, aiNode** children, unsigned int numChildren, RenderableEntity& parentRE);
	void LoadNodeMeshes(const aiScene* scene, unsigned int* meshesIndices, unsigned int numMeshes, RenderableEntity& parentRE);
	Assimp::Importer importer;

	SubmeshInfo& findSubmeshInfo(SubmeshHandle_t sHnd);
	void ParseSceneGraph(const Node<RenderableEntity>* node, const dx::XMMATRIX& parentTransf, std::vector<SubmeshInfo>& submeshesInView);
};


extern RendererFrontend rf;

#endif // !RENDERER_FRONTEND_H_
