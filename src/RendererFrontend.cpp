#include "RendererFrontend.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "DirectXMath.h"

#include <cassert>
#include <iostream>
#include <winerror.h>

#include "Model.h"
#include "BufferCache.h"
#include "Material.h"
#include "ResourceCache.h"
#include "Camera.h"
#include "SceneGraph.h"

namespace dx = DirectX;

RendererFrontend rf;

dx::XMMATRIX buildDXTransform(aiMatrix4x4 assTransf) {
    aiVector3D scal;
    aiQuaternion quat;
    aiVector3D pos;
    assTransf.Decompose(scal, quat, pos);

    dx::XMMATRIX transMat = dx::XMMatrixTranslation(pos.x, pos.y, pos.z);
    dx::XMMATRIX rotMat = dx::XMMatrixRotationQuaternion(dx::XMVectorSet(quat.x, quat.y, quat.z, quat.w));
    dx::XMMATRIX scalMat = dx::XMMatrixScaling(scal.x, scal.y, scal.z);
    return dx::XMMatrixMultiply(transMat, dx::XMMatrixMultiply(rotMat, scalMat));
}

RendererFrontend::RendererFrontend() : sceneGraph( RenderableEntity{ 0,SubmeshHandle_t{0,0,0},"scene", dx::XMMatrixIdentity() } ) {}
   
RendererFrontend::~RendererFrontend()
{
}

void RendererFrontend::InitScene()
{
}

//TODO: move somewhere else and use other hashing function; this is temporary
inline uint32_t hash_str_uint32(const std::string& str) {

    uint32_t hash = 0x811c9dc5;
    uint32_t prime = 0x1000193;

    for (int i = 0; i < str.size(); ++i) {
        uint8_t value = str[i];
        hash = hash ^ value;
        hash *= prime;
    }

    return hash;

}

RenderableEntity RendererFrontend::LoadModelFromFile(const std::string& filename, bool forceFlattenScene)
{
    
    const aiScene* scene = importer.ReadFile(filename, 
        aiProcess_FindInvalidData |
        aiProcess_GenSmoothNormals | 
        aiProcess_Triangulate | 
        aiProcess_JoinIdenticalVertices | 
        aiProcess_CalcTangentSpace |
        aiProcess_GenBoundingBoxes |
        (forceFlattenScene ? aiProcess_PreTransformVertices : 0) |
        aiProcess_ConvertToLeftHanded
    );

    aiReturn success;

    RenderableEntity rEnt{ 0,0,0,0 };
    SubmeshHandle_t submeshHandle{ 0,0,0,0 };

    if (scene == nullptr)
    {
        std::cout << importer.GetErrorString() << std::endl;
        return rEnt;
    }
    
    float unitScaleFactor = 1.0f;
    if (scene->mMetaData->HasKey("UnitScaleFactor")) {
        scene->mMetaData->Get<float>("UnitScaleFactor", unitScaleFactor);
    }

    aiNode* rootNode = scene->mRootNode;
    
    dx::XMMATRIX localTransf = buildDXTransform(rootNode->mTransformation);
    rEnt = RenderableEntity{ hash_str_uint32(scene->GetShortFilename(filename.c_str())), SubmeshHandle_t{0,0,0}, scene->GetShortFilename(filename.c_str()), localTransf };
    sceneGraph.insertNode(RenderableEntity{ 0 }, rEnt);

    LoadNodeChildren(scene, rootNode->mChildren, rootNode->mNumChildren, rEnt);

	return rEnt;
}


void RendererFrontend::LoadNodeChildren(const aiScene* scene, aiNode** children, unsigned int numChildren, RenderableEntity& parentRE)
{
    for (int i = 0; i < numChildren; i++) {
        dx::XMMATRIX localTransf = buildDXTransform(children[i]->mTransformation);

        RenderableEntity rEnt{ hash_str_uint32(children[i]->mName.C_Str()), SubmeshHandle_t{0,0,0}, children[i]->mName.C_Str(), localTransf };
        sceneGraph.insertNode(parentRE, rEnt);

        LoadNodeMeshes(scene, children[i]->mMeshes, children[i]->mNumMeshes, rEnt);

        if (children[i]->mNumChildren > 0) {
            LoadNodeChildren(scene, children[i]->mChildren, children[i]->mNumChildren, rEnt);
        }
    }
}

void RendererFrontend::LoadNodeMeshes(const aiScene* scene, unsigned int* meshesIndices, unsigned int numMeshes, RenderableEntity& parentRE)
{
    //assert(numMeshes <= 1);
    aiReturn success;

    SubmeshHandle_t submeshHandle{ 0,0,0,0 };

    for (int j = 0; j < numMeshes; j++) {
        //assert(numMeshes <= 1);

        aiMesh* mesh = scene->mMeshes[meshesIndices[j]];

        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        aiColor3D diffCol(0.0f, 0.0f, 0.0f);
        if (AI_SUCCESS != material->Get(AI_MATKEY_COLOR_DIFFUSE, diffCol)) {
            OutputDebugString(importer.GetErrorString());
        }

        MaterialDesc matDesc;
        matDesc.shaderType = SHADER_TYPE::STANDARD;

        aiString diffTexName;
        int count = material->GetTextureCount(aiTextureType_DIFFUSE);
        if (count > 0) {
            success = material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), diffTexName);
            if (!success) {
                OutputDebugString(importer.GetErrorString());
            }

        }

        //TODO: implement reading material info in material desc
        aiColor4D col;
        success = material->Get(AI_MATKEY_BASE_COLOR, col);
        matDesc.baseColor = dx::XMFLOAT4(col.r, col.g, col.b, col.a);
        matDesc.smoothness = 0.5f;
        matDesc.metalness = 0.0f;
        matDesc.albedoPath = L""; // L"./shaders/assets/Human/Textures/Head/JPG/Colour_8k.jpg";
        matDesc.normalPath = L""; // L"./shaders/assets/Human/Textures/Head/JPG/Normal Map_SubDivision_1.jpg";
        matDesc.metalnessMask = L"";

        materials.push_back(matDesc);

        submeshHandle.materialIdx = materials.size() - 1;
        submeshHandle.numVertices = mesh->mNumVertices;
        submeshHandle.baseVertex = staticSceneVertexData.size();

        //TODO:hmmm? evaluate should if i reserve in advance? when?
        staticSceneVertexData.reserve((size_t)submeshHandle.numVertices + (size_t)submeshHandle.baseVertex);

        for (int i = 0; i < submeshHandle.numVertices; i++)
        {
            aiVector3D* vert = &(mesh->mVertices[i]);
            aiVector3D* normal = &(mesh->mNormals[i]);
            aiVector3D* textureCoord = &mesh->mTextureCoords[0][i];
            aiVector3D* tangent = &mesh->mTangents[i];

            staticSceneVertexData.push_back(Vertex(vert->x , vert->y, vert->z, textureCoord->x, textureCoord->y, normal->x, normal->y, normal->z, tangent->x, tangent->y, tangent->z));
        }

        int numFaces = mesh->mNumFaces;
        submeshHandle.numIndexes = numFaces * 3;
        submeshHandle.baseIndex = staticSceneIndexData.size();

        staticSceneIndexData.reserve((size_t)submeshHandle.baseIndex + (size_t)submeshHandle.numIndexes);

        for (int i = 0; i < numFaces; i++) {
            aiFace* face = &(mesh->mFaces[i]);
            for (int j = 0; j < face->mNumIndices; j++) {
                staticSceneIndexData.push_back(face->mIndices[j]);
            }
        }

        //TODO: maybe implement move for submeshHandle?
        sceneMeshes.push_back(SubmeshInfo{ submeshHandle, BufferCacheHandle_t{0,0}, MaterialCacheHandle_t{0,0}, dx::XMMatrixIdentity() });

        RenderableEntity rEnt{ hash_str_uint32(std::to_string(submeshHandle.baseVertex)), submeshHandle, mesh->mName.C_Str(), dx::XMMatrixIdentity() };
        sceneGraph.insertNode(parentRE, rEnt);
    }
}

SubmeshInfo& RendererFrontend::findSubmeshInfo(SubmeshHandle_t sHnd) {
    for (SubmeshInfo& sbInfo : sceneMeshes) {
        //TODO: make better comparison
        if (sbInfo.submeshHnd.baseVertex == sHnd.baseVertex) {
            return sbInfo;
        }
    }
    assert(false);
}

void RendererFrontend::ParseSceneGraph(const Node<RenderableEntity>* node, const dx::XMMATRIX& parentTransf, std::vector<SubmeshInfo>& submeshesInView) {
    dx::XMMATRIX newTransf = dx::XMMatrixMultiply(node->value.localWorldTransf, parentTransf);
    if (node->value.submeshHnd.numVertices > 0) {
        SubmeshInfo& sbPair = findSubmeshInfo(node->value.submeshHnd);
        //TODO: implement frustum culling

        bool cached = sbPair.bufferHnd.isCached; //bufferCache.isCached(sHnd);
        if (!cached) {
            sbPair.bufferHnd = bufferCache.AllocStaticGeom(sbPair.submeshHnd, staticSceneIndexData.data() + sbPair.submeshHnd.baseIndex, staticSceneVertexData.data() + sbPair.submeshHnd.baseVertex);
        }

        cached = sbPair.matHnd.isCached;
        if (!cached) {
            sbPair.matHnd = resourceCache.AllocMaterial(materials.at(sbPair.submeshHnd.materialIdx));
        }
        sbPair.finalWorldTransf = newTransf;
        submeshesInView.push_back(sbPair);
        
    }
    
    for (Node<RenderableEntity>* n : node->children) {
        ParseSceneGraph(n, newTransf, submeshesInView);
    }
    
}

ViewDesc RendererFrontend::ComputeView(const Camera& cam)
{
    std::vector<SubmeshInfo> submeshesInView;
    submeshesInView.reserve(sceneMeshes.size());
    
    Node<RenderableEntity>* root = sceneGraph.rootNode;

    ParseSceneGraph(root, dx::XMMatrixIdentity(), submeshesInView);

    //TODO: does move do what I was expecting?
    return ViewDesc{std::move(submeshesInView), cam};
}