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

struct Transform_t {
	dx::XMFLOAT3 pos;
	dx::XMFLOAT3 rot;
	dx::XMFLOAT3 scal;
};

constexpr Transform_t IDENTITY_TRANSFORM{ dx::XMFLOAT3(0.0f,0.0f,0.0f), dx::XMFLOAT3(0.0f,0.0f,0.0f), dx::XMFLOAT3(1.0f,1.0f,1.0f) };

struct RenderableEntity {
	bool operator==(const RenderableEntity& r) {
		return ID == r.ID;
	}

	std::uint32_t ID;
	SubmeshHandle_t submeshHnd;
	std::string entityName;
	Transform_t localWorldTransf;
	//dx::XMMATRIX localWorldTransf;
	//Bound aabb;
};

struct SubmeshInfo {
	SubmeshHandle_t submeshHnd;
	BufferCacheHandle_t bufferHnd;
	MaterialCacheHandle_t matCacheHnd;
	dx::XMMATRIX finalWorldTransf;
};

struct ViewDesc {
	std::vector<SubmeshInfo> submeshesInfo;
	const Camera cam;
};

class RendererFrontend {

public:
	RendererFrontend();

	//TODO: probably move somewhere else
	RenderableEntity LoadModelFromFile(const std::string& filename, bool optimizeGraph = false, bool forceFlattenScene = false);
	ViewDesc ComputeView(const Camera& cam);

	SceneGraph<RenderableEntity> sceneGraph;

	//TODO: Maybe should be BVH instead of linear list? Remember Frostbite presentation.
	std::vector<SubmeshInfo> sceneMeshes;  //scene meshes indexed by model handle
	
	std::vector<MaterialDesc> materials; //material indexed by part of submesh handle
	std::vector<Vertex> staticSceneVertexData; //mesh vertex data base indexed by part of submesh handle
	std::vector<std::uint16_t> staticSceneIndexData; //mesh index data base indexed by part of submesh handle

	SubmeshInfo* findSubmeshInfo(SubmeshHandle_t sHnd);

private:
	void LoadNodeChildren(const aiScene* scene, aiNode** children, unsigned int numChildren, RenderableEntity& parentRE);
	RenderableEntity LoadNodeMeshes(const aiScene* scene, unsigned int* meshesIndices, unsigned int numMeshes, RenderableEntity& parentRE, Transform_t localTransf = IDENTITY_TRANSFORM);
	Assimp::Importer importer;

	void ParseSceneGraph(const Node<RenderableEntity>* node, const dx::XMMATRIX& parentTransf, std::vector<SubmeshInfo>& submeshesInView);
};


extern RendererFrontend rf;

#endif // !RENDERER_FRONTEND_H_
