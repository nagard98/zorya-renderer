#include "RendererFrontend.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "DirectXMath.h"

#include <iostream>
#include <winerror.h>

#include "Model.h"
#include "BufferCache.h"
#include "Material.h"
#include "ResourceCache.h"

namespace dx = DirectX;

RendererFrontend rf;

RendererFrontend::RendererFrontend()
{
}

RendererFrontend::~RendererFrontend()
{
}

void RendererFrontend::InitScene()
{
}

ModelHandle_t RendererFrontend::LoadModelFromFile(const std::string& filename)
{
    const aiScene* scene = importer.ReadFile(filename, 
        aiProcess_GenSmoothNormals | 
        aiProcess_Triangulate | 
        aiProcess_JoinIdenticalVertices | 
        aiProcess_CalcTangentSpace |
        aiProcess_GenBoundingBoxes | 
        aiProcess_PreTransformVertices |
        aiProcess_ConvertToLeftHanded
    );

    aiReturn success;

    ModelHandle_t modelHandle{ 0, 0 };
    SubmeshHandle_t submeshHandle{ 0,0,0,0 };

    if (scene == nullptr)
    {
        std::cout << importer.GetErrorString() << std::endl;
        return modelHandle;
    }
    
    int numMeshes = scene->mNumMeshes;
    for (int j = 0; j < numMeshes; j++) {

        aiMesh* mesh = scene->mMeshes[j];
        aiMatrix4x4 sM = scene->mRootNode->mTransformation;

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
        matDesc.baseColor = dx::XMFLOAT4(0.5f, 0.0f,0.0f,1.0f);
        matDesc.smoothness = 0.5f;
        matDesc.metalness = 0.0f;
        matDesc.albedoPath = L"./shaders/assets/Human/Textures/Head/JPG/Colour_8k.jpg";
        matDesc.normalPath = L"./shaders/assets/Human/Textures/Head/JPG/Normal Map_SubDivision_1.jpg";
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

            staticSceneVertexData.push_back(Vertex(vert->x, vert->y, vert->z, textureCoord->x, textureCoord->y, normal->x, normal->y, normal->z, tangent->x, tangent->y, tangent->z));
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
        sceneMeshes.push_back(SubmeshInfo{ submeshHandle, BufferCacheHandle_t{0,0}, MaterialCacheHandle_t{0,0} });
    }

    //TODO: maybe offset is not correct; check
    modelHandle.offsetToStart = sceneMeshes.size();
    modelHandle.numMeshes = numMeshes;

	return modelHandle;
}

ViewDesc RendererFrontend::ComputeView()
{
    std::vector<SubmeshInfo> submeshesInView;
    submeshesInView.reserve(sceneMeshes.size());

    //TODO: implement frustum culling
    for (SubmeshInfo &sbPair : sceneMeshes) {
        bool cached = sbPair.bufferHnd.isCached; //bufferCache.isCached(sHnd);
        if (!cached) {
            sbPair.bufferHnd = bufferCache.AllocStaticGeom(sbPair.submeshHnd, staticSceneIndexData.data() + sbPair.submeshHnd.baseIndex , staticSceneVertexData.data() + sbPair.submeshHnd.baseVertex);
        }

        cached = sbPair.matHnd.isCached;
        if (!cached) {
            sbPair.matHnd = resourceCache.AllocMaterial(materials.at(sbPair.submeshHnd.materialIdx));
        }

        submeshesInView.push_back(sbPair);
    }

    //TODO: does move do what I was expecting?
    return ViewDesc{std::move(submeshesInView)};
}
