#ifndef RENDERER_FRONTEND_H_
#define RENDERER_FRONTEND_H_

#include "Model.h"
#include "BufferCache.h"
#include "ResourceCache.h"
#include "Camera.h"
#include "SceneGraph.h"
#include "Lights.h"
#include "Transform.h"

#include <DirectXMath.h>

#include "assimp/scene.h"
#include "assimp/Importer.hpp"

#include <string>
#include <cstdint>

namespace dx = DirectX;

//struct RenderableEntityHandle_t {
//	std::uint32_t index;
//	std::uint32_t numChildren;
//	std::uint8_t isValid;
//};

//struct Transform_t {
//	dx::XMFLOAT3 pos;
//	dx::XMFLOAT3 rot;
//	dx::XMFLOAT3 scal;
//};
//
//constexpr Transform_t IDENTITY_TRANSFORM{ dx::XMFLOAT3(0.0f,0.0f,0.0f), dx::XMFLOAT3(0.0f,0.0f,0.0f), dx::XMFLOAT3(1.0f,1.0f,1.0f) };

enum class EntityType : std::uint8_t{
	UNDEFINED,
	MESH,
	LIGHT,
	CAMERA,
	COLLECTION
};

struct FragmentInfo {
	std::uint32_t index;
	std::uint32_t length;
};

struct GenericEntityHandle {
	EntityType tag;
	union {
		SubmeshHandle_t submeshHnd;
		LightHandle_t lightHnd;
	};
};

struct RenderableEntity {
	bool operator==(const RenderableEntity& r) {
		return ID == r.ID;
	}

	std::uint32_t ID;
	EntityType tag;
	union {
		SubmeshHandle_t submeshHnd;
		LightHandle_t lightHnd;
	};
	std::string entityName;
	Transform_t localWorldTransf;
	//Bound aabb;
};

struct SubmeshInfo {
	SubmeshHandle_t submeshHnd;
	BufferCacheHandle_t bufferHnd;
	MaterialCacheHandle_t matCacheHnd;
	dx::XMMATRIX finalWorldTransf;
};

struct LightInfo {
	LightType tag;
	union {
		DirectionalLight dirLight;
		PointLight pointLight;
		SpotLight spotLight;
	};
	dx::XMMATRIX finalWorldTransf;
};

struct ViewDesc {
	std::vector<SubmeshInfo> submeshesInfo;
	std::vector<LightInfo> lightsInfo;
	std::uint8_t numDirLights;
	std::uint8_t numPointLights;
	std::uint8_t numSpotLights;
	const Camera cam;
};

class RendererFrontend {

public:
	RendererFrontend();

	//TODO: probably move somewhere else
	RenderableEntity LoadModelFromFile(const std::string& filename, bool optimizeGraph = false, bool forceFlattenScene = false);
	RenderableEntity AddLight(const RenderableEntity* attachTo, dx::XMVECTOR direction, float shadowMapNearPlane = 2.0f, float shadowMapFarPlane = 20.0f); //add directional light
	RenderableEntity AddLight(const RenderableEntity* attachTo, dx::XMVECTOR direction, dx::XMVECTOR position, float cutoffAngle); //add spotlight
	RenderableEntity AddLight(const RenderableEntity* attachTo, dx::XMVECTOR position, float constant, float linear, float quadratic); //add point light

	int removeEntity(RenderableEntity& entityToRemove);

	ViewDesc ComputeView(const Camera& cam);

	SubmeshInfo* findSubmeshInfo(SubmeshHandle_t sHnd);


	SceneGraph<RenderableEntity> sceneGraph;

	//TODO: Maybe should be BVH instead of linear list? Remember Frostbite presentation.
	std::vector<SubmeshInfo> sceneMeshes;  //scene meshes indexed by model handle
	std::vector<LightInfo> sceneLights;
	
	std::vector<MaterialDesc> materials; //material indexed by part of submesh handle
	std::vector<Vertex> staticSceneVertexData; //mesh vertex data base indexed by part of submesh handle
	std::vector<std::uint16_t> staticSceneIndexData; //mesh index data base indexed by part of submesh handle

private:
	void LoadNodeChildren(const aiScene* scene, aiNode** children, unsigned int numChildren, RenderableEntity& parentRE);
	RenderableEntity LoadNodeMeshes(const aiScene* scene, unsigned int* meshesIndices, unsigned int numMeshes, RenderableEntity& parentRE, Transform_t localTransf = IDENTITY_TRANSFORM);

	void ParseSceneGraph(const Node<RenderableEntity>* node, const dx::XMMATRIX& parentTransf, std::vector<SubmeshInfo>& submeshesInView, std::vector<LightInfo>& lightsInView);

	Assimp::Importer importer;
	std::vector<int> freedSceneLightIndices;
	std::vector<int> freedSceneMeshIndices;
	std::vector<int> freedSceneMaterialIndices;

	std::vector<FragmentInfo> freedStaticSceneVertexDataFragments;
	std::vector<FragmentInfo> freedStaticSceneIndexDataFragments;
};


extern RendererFrontend rf;

#endif // !RENDERER_FRONTEND_H_
