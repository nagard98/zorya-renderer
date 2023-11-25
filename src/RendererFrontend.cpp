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
#include "Shaders.h"

#include <cstdlib>

namespace dx = DirectX;

RendererFrontend rf;

dx::XMMATRIX mult(const Transform_t& a, const Transform_t& b) {
    dx::XMMATRIX matTransfA = dx::XMMatrixMultiply(dx::XMMatrixRotationRollPitchYaw(a.rot.x, a.rot.y, a.rot.z), dx::XMMatrixScaling(a.scal.x, a.scal.y, a.scal.z));
    matTransfA = dx::XMMatrixMultiply(dx::XMMatrixTranslation(a.pos.x, a.pos.y, a.pos.z), matTransfA);
    
    dx::XMMATRIX matTransfB = dx::XMMatrixMultiply(dx::XMMatrixRotationRollPitchYaw(b.rot.x, b.rot.y, b.rot.z), dx::XMMatrixScaling(b.scal.x, b.scal.y, b.scal.z));
    matTransfB = dx::XMMatrixMultiply(dx::XMMatrixTranslation(b.pos.x, b.pos.y, b.pos.z), matTransfB);

    return dx::XMMatrixMultiply(matTransfA, matTransfB);
}

dx::XMMATRIX mult(const Transform_t& a, const dx::XMMATRIX& b) {
    dx::XMMATRIX matTransfA = dx::XMMatrixMultiply(dx::XMMatrixRotationRollPitchYaw(a.rot.x, a.rot.y, a.rot.z), dx::XMMatrixScaling(a.scal.x, a.scal.y, a.scal.z));
    matTransfA = dx::XMMatrixMultiply(dx::XMMatrixTranslation(a.pos.x, a.pos.y, a.pos.z), matTransfA);

    return dx::XMMatrixMultiply(matTransfA, b);
}

Transform_t buildDXTransform(aiMatrix4x4 assTransf) {
    aiVector3D scal;
    aiQuaternion quat;
    aiVector3D pos;
    aiVector3D rot;
    assTransf.Decompose(scal, quat, pos);
    assTransf.Decompose(scal, rot, pos);

    dx::XMMATRIX transMat = dx::XMMatrixTranslation(pos.x, pos.y, pos.z);
    dx::XMMATRIX rotMat = dx::XMMatrixRotationQuaternion(dx::XMVectorSet(quat.x, quat.y, quat.z, quat.w));
    dx::XMMATRIX scalMat = dx::XMMatrixScaling(scal.x, scal.y, scal.z);
    //return dx::XMMatrixMultiply(transMat, dx::XMMatrixMultiply(rotMat, scalMat));
    return Transform_t{ dx::XMFLOAT3{pos.x, pos.y, pos.z}, dx::XMFLOAT3{rot.x, rot.y, rot.z}, dx::XMFLOAT3{scal.x, scal.y, scal.z} };
}

RendererFrontend::RendererFrontend() : sceneGraph( RenderableEntity{ 0,SubmeshHandle_t{0,0,0},"scene", IDENTITY_TRANSFORM } ) {}
   
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

    importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, 65535);
    importer.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, 65535);

    const aiScene* scene = importer.ReadFile(filename, 
        aiProcess_FindInvalidData |
        aiProcess_GenSmoothNormals | 
        aiProcess_Triangulate | 
        aiProcess_JoinIdenticalVertices | 
        aiProcess_CalcTangentSpace |
        aiProcess_GenBoundingBoxes |
        //aiProcess_OptimizeGraph |
        (forceFlattenScene ? aiProcess_PreTransformVertices : 0) |
        aiProcess_ConvertToLeftHanded
    );

    aiReturn success;

    RenderableEntity rootEnt{ 0,0,0,0 };
    SubmeshHandle_t submeshHandle{ 0,0,0,0 };

    if (scene == nullptr)
    {
        std::cout << importer.GetErrorString() << std::endl;
        return rootEnt;
    }
    
    float unitScaleFactor = 1.0f;
    if (scene->mMetaData->HasKey("UnitScaleFactor")) {
        scene->mMetaData->Get<float>("UnitScaleFactor", unitScaleFactor);
    }

    aiNode* rootNode = scene->mRootNode;
    
    Transform_t localTransf = buildDXTransform(rootNode->mTransformation);
    rootEnt = RenderableEntity{ hash_str_uint32(scene->GetShortFilename(filename.c_str())), SubmeshHandle_t{0,0,0}, scene->GetShortFilename(filename.c_str()), localTransf };
    sceneGraph.insertNode(RenderableEntity{ 0 }, rootEnt);
    
    materials.resize(materials.size() + scene->mNumMaterials);
    size_t sepIndex = filename.find_last_of("/");
    aiString basePath = aiString(filename.substr(0, sepIndex));
    scene->mMetaData->Add("basePath", basePath);

    //RenderableEntity rEnt{};
    //if (rootNode->mNumMeshes == 1) {
    //    rEnt = LoadNodeMeshes(scene, rootNode->mMeshes, rootNode->mNumMeshes, rootEnt, localTransf);
    //}
    //else {
    //    rEnt = RenderableEntity{ hash_str_uint32(rootNode->mName.C_Str()), SubmeshHandle_t{ 0,0,0 }, rootNode->mName.C_Str(), localTransf };
    //    sceneGraph.insertNode(rootEnt, rEnt);
    //    LoadNodeMeshes(scene, rootNode->mMeshes, rootNode->mNumMeshes, rEnt);
    //}
    
    LoadNodeChildren(scene, rootNode->mChildren, rootNode->mNumChildren, rootEnt);

	return rootEnt;
}


void RendererFrontend::LoadNodeChildren(const aiScene* scene, aiNode** children, unsigned int numChildren, RenderableEntity& parentRE)
{
    for (int i = 0; i < numChildren; i++) {
        Transform_t localTransf = buildDXTransform(children[i]->mTransformation);

        RenderableEntity rEnt;
        if (children[i]->mNumMeshes == 1) {
            rEnt = LoadNodeMeshes(scene, children[i]->mMeshes, children[i]->mNumMeshes, parentRE, localTransf);
        }
        else {
            rEnt = RenderableEntity{ hash_str_uint32(children[i]->mName.C_Str()), SubmeshHandle_t{ 0,0,0 }, children[i]->mName.C_Str(), localTransf };
            sceneGraph.insertNode(parentRE, rEnt);
            LoadNodeMeshes(scene, children[i]->mMeshes, children[i]->mNumMeshes, rEnt);
        }

        LoadNodeChildren(scene, children[i]->mChildren, children[i]->mNumChildren, rEnt);
        
    }
}

RenderableEntity RendererFrontend::LoadNodeMeshes(const aiScene* scene, unsigned int* meshesIndices, unsigned int numMeshes, RenderableEntity& parentRE, const Transform_t& localTransf)
{
    aiReturn success;

    SubmeshHandle_t submeshHandle{ 0,0,0,0 };
    RenderableEntity rEnt{};

    aiString basePath;
    scene->mMetaData->Get("basePath", basePath);

    for (int j = 0; j < numMeshes; j++) {
        aiMesh* mesh = scene->mMeshes[meshesIndices[j]];

        //TODO: implement reading material info in material desc-------------------------
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        std::uint16_t matDescId = materials.size() - scene->mNumMaterials + mesh->mMaterialIndex;
        MaterialDesc& matDesc = materials.at(matDescId);
        MaterialCacheHandle_t initMatCacheHnd{ 0, NO_UPDATE_MAT};

        if (matDesc.shaderType == PShaderID::UNDEFINED) {
            wchar_t tmpString[128];

            aiColor3D diffCol(0.0f, 0.0f, 0.0f);
            if (AI_SUCCESS != material->Get(AI_MATKEY_COLOR_DIFFUSE, diffCol)) {
                OutputDebugString(importer.GetErrorString());
            }

            matDesc.shaderType = PShaderID::STANDARD;

            aiString diffTexName;
            int count = material->GetTextureCount(aiTextureType_DIFFUSE);
            if (count > 0) {
                success = material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), diffTexName);
                if (!(aiReturn_SUCCESS == success)) {
                    OutputDebugString(importer.GetErrorString());
                }
                else {
                    aiString albedoPath = aiString(basePath);
                    albedoPath.Append("/");
                    albedoPath.Append(diffTexName.C_Str());
                    mbstowcs(tmpString, albedoPath.C_Str(), 128);
                    wcscpy(matDesc.albedoPath, tmpString); // L"./shaders/assets/Human/Textures/Head/JPG/Colour_8k.jpg";
                }
            }

            aiString normTexName;
            count = material->GetTextureCount(aiTextureType_NORMALS);
            if (count > 0) {
                success = material->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), normTexName);
                if (!(aiReturn_SUCCESS == success)) {
                    OutputDebugString(importer.GetErrorString());
                }
                else {
                    aiString NormalMapPath = aiString(basePath);
                    NormalMapPath.Append("/");
                    NormalMapPath.Append(normTexName.C_Str());
                    mbstowcs(tmpString, NormalMapPath.C_Str(), 128);
                    wcscpy(matDesc.normalPath, tmpString);
                    // L"./shaders/assets/Human/Textures/Head/JPG/Normal Map_SubDivision_1.jpg";
                }
            }

            aiColor4D col;
            success = material->Get(AI_MATKEY_COLOR_DIFFUSE, col);
            if (aiReturn_SUCCESS == success) {
                matDesc.baseColor = dx::XMFLOAT4(col.r, col.g, col.b, col.a);
            }
            else {
                matDesc.baseColor = dx::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
            }

            float roughness = 0.0f;
            success = material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
            if (aiReturn_SUCCESS == success) {
                //TODO:: do correct conversion roughness surface
                matDesc.smoothness = roughness;
            }
            else {
                matDesc.smoothness = 0.5f;
            }

            float metalness = 0.0f;
            success = material->Get(AI_MATKEY_METALLIC_FACTOR, metalness);
            if (aiReturn_SUCCESS == success) {
                matDesc.metalness = metalness;
            }
            else {
                matDesc.metalness = 0.0f;
            }


            wcscpy(matDesc.metalnessMask, L"");

            initMatCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS | IS_FIRST_MAT_ALLOC;
        }
        //---------------------------------------------------

        submeshHandle.matDescIdx = matDescId;
        submeshHandle.numVertices = mesh->mNumVertices;
        submeshHandle.baseVertex = staticSceneVertexData.size();

        //TODO: hmmm? evaluate should if i reserve in advance? when?
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
        sceneMeshes.emplace_back(SubmeshInfo{ submeshHandle, BufferCacheHandle_t{0,0}, initMatCacheHnd, dx::XMMatrixIdentity() });

        rEnt = RenderableEntity{ hash_str_uint32(std::to_string(submeshHandle.baseVertex)), submeshHandle, mesh->mName.C_Str(), localTransf };
        sceneGraph.insertNode(parentRE, rEnt);
    }

    return rEnt;
}

SubmeshInfo* RendererFrontend::findSubmeshInfo(SubmeshHandle_t sHnd) {
    for (SubmeshInfo& sbInfo : sceneMeshes) {
        //TODO: make better comparison
        if (sbInfo.submeshHnd.baseVertex == sHnd.baseVertex && sbInfo.submeshHnd.numVertices == sHnd.numVertices) {
            return &sbInfo;
        }
    }
    return nullptr;
}

void RendererFrontend::ParseSceneGraph(const Node<RenderableEntity>* node, const dx::XMMATRIX& parentTransf, std::vector<SubmeshInfo>& submeshesInView) {
    dx::XMMATRIX newTransf = mult(node->value.localWorldTransf, parentTransf);
    if (node->value.submeshHnd.numVertices > 0) {
        SubmeshInfo* sbPair = findSubmeshInfo(node->value.submeshHnd);
        //TODO: implement frustum culling

        bool cached = sbPair->bufferHnd.isCached; 
        if (!cached) {
            sbPair->bufferHnd = bufferCache.AllocStaticGeom(sbPair->submeshHnd, staticSceneIndexData.data() + sbPair->submeshHnd.baseIndex, staticSceneVertexData.data() + sbPair->submeshHnd.baseVertex);
        }

        cached = (sbPair->matCacheHnd.isCached & (UPDATE_MAT_MAPS | UPDATE_MAT_PRMS)) == 0;
        if (!cached) {
            sbPair->matCacheHnd = resourceCache.AllocMaterial(materials.at(sbPair->submeshHnd.matDescIdx), sbPair->matCacheHnd);
        }
        sbPair->finalWorldTransf = newTransf;
        submeshesInView.push_back(*sbPair);
        
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
