#include "RendererFrontend.h"

#include "Editor/Logger.h"

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
#include "Lights.h"
#include "Transform.h"

#include <cstdlib>

namespace dx = DirectX;

RendererFrontend rf;

Transform_t buildTransform(aiMatrix4x4 assTransf) {
    aiVector3D scal;
    aiVector3D pos;
    aiVector3D rot;
    assTransf.Decompose(scal, rot, pos);

    return Transform_t{ dx::XMFLOAT3{pos.x, pos.y, pos.z}, dx::XMFLOAT3{rot.x, rot.y, rot.z}, dx::XMFLOAT3{scal.x, scal.y, scal.z} };
}

RendererFrontend::RendererFrontend() : sceneGraph(RenderableEntity{ 0,EntityType::COLLECTION, SubmeshHandle_t{0,0,0},"scene", IDENTITY_TRANSFORM }) {}
   
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

RenderableEntity RendererFrontend::LoadModelFromFile(const std::string& filename, bool optimizeGraph, bool forceFlattenScene)
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
        (optimizeGraph ? aiProcess_OptimizeGraph : 0) |
        (forceFlattenScene ? aiProcess_PreTransformVertices : 0) |
        aiProcess_ConvertToLeftHanded
    );

    aiReturn success;

    RenderableEntity rootEnt{ 0,EntityType::UNDEFINED,0,0,0 };
    SubmeshHandle_t submeshHandle{ 0,0,0,0 };

    if (scene == nullptr)
    {
        Logger::AddLog(Logger::Channel::ERR, "%s\n", importer.GetErrorString());
        return rootEnt;
    }

    aiNode* rootNode = scene->mRootNode;

    materials.resize(materials.size() + scene->mNumMaterials);
    size_t sepIndex = filename.find_last_of("/");
    aiString basePath = aiString(filename.substr(0, sepIndex));
    scene->mMetaData->Add("basePath", basePath);

    LoadNodeChildren(scene, &rootNode, 1, RenderableEntity{ 0 });

    Logger::AddLog(Logger::Channel::TRACE, "Imported model %s\n", scene->GetShortFilename(filename.c_str()));

	return rootEnt;
}

RenderableEntity RendererFrontend::AddLight(const RenderableEntity* attachTo, dx::XMVECTOR direction)
{
        LightHandle_t lightHnd;
        lightHnd.index = sceneLights.size();
        lightHnd.tag = LightType::DIRECTIONAL;
        sceneLights.emplace_back(LightInfo{ LightType::DIRECTIONAL, DirectionalLight{direction}, dx::XMMatrixIdentity() });

        //TODO:decide what to hash for id
        RenderableEntity rEnt;
        rEnt.ID= hash_str_uint32(std::string("dir_light_")+std::to_string( sceneLights.size()));
        rEnt.entityName = "Dir Light";
        rEnt.lightHnd = lightHnd;
        rEnt.tag = EntityType::LIGHT;
        rEnt.localWorldTransf = IDENTITY_TRANSFORM;

    if (attachTo == nullptr) {
        sceneGraph.insertNode(RenderableEntity{ 0 }, rEnt);
        return rEnt;
    }
    else {
        assert(false);
    }

}

RenderableEntity RendererFrontend::AddLight(const RenderableEntity* attachTo, dx::XMVECTOR direction, dx::XMVECTOR position, float cutoffAngle)
{
    LightHandle_t lightHnd;
    lightHnd.index = sceneLights.size();
    lightHnd.tag = LightType::SPOT;

    LightInfo& lightInfo = sceneLights.emplace_back();
    lightInfo.tag = LightType::SPOT;
    lightInfo.spotLight = SpotLight{ position,direction, std::cos(cutoffAngle) };
    lightInfo.finalWorldTransf = dx::XMMatrixIdentity();

    //TODO:decide what to hash for id
    RenderableEntity rEnt;
    rEnt.ID = hash_str_uint32(std::string("spot_light_") + std::to_string(sceneLights.size()));
    rEnt.entityName = "Spot Light";
    rEnt.lightHnd = lightHnd;
    rEnt.tag = EntityType::LIGHT;
    rEnt.localWorldTransf = IDENTITY_TRANSFORM;

    if (attachTo == nullptr) {
        sceneGraph.insertNode(RenderableEntity{ 0 }, rEnt);
        return rEnt;
    }
    else {
        assert(false);
    }

}

RenderableEntity RendererFrontend::AddLight(const RenderableEntity* attachTo, dx::XMVECTOR position, float constant, float linear, float quadratic)
{
    LightHandle_t lightHnd;
    lightHnd.index = sceneLights.size();
    lightHnd.tag = LightType::POINT;

    LightInfo& lightInfo = sceneLights.emplace_back();
    lightInfo.tag = LightType::POINT;
    lightInfo.pointLight = PointLight{ position, constant, linear, quadratic};
    lightInfo.finalWorldTransf = dx::XMMatrixIdentity();

    //TODO:decide what to hash for id
    RenderableEntity rEnt;
    rEnt.ID = hash_str_uint32(std::string("point_light_") + std::to_string(sceneLights.size()));
    rEnt.entityName = "Point Light";
    rEnt.lightHnd = lightHnd;
    rEnt.tag = EntityType::LIGHT;
    rEnt.localWorldTransf = IDENTITY_TRANSFORM;

    if (attachTo == nullptr) {
        sceneGraph.insertNode(RenderableEntity{ 0 }, rEnt);
        return rEnt;
    }
    else {
        assert(false);
    }
}


void RendererFrontend::LoadNodeChildren(const aiScene* scene, aiNode** children, unsigned int numChildren, RenderableEntity& parentRE)
{
    for (int i = 0; i < numChildren; i++) {
        Transform_t localTransf = buildTransform(children[i]->mTransformation);

        RenderableEntity rEnt;
        if (children[i]->mNumMeshes == 1) {
            rEnt = LoadNodeMeshes(scene, children[i]->mMeshes, children[i]->mNumMeshes, parentRE, localTransf);
        }
        else {
            rEnt = RenderableEntity{ hash_str_uint32(children[i]->mName.C_Str()), EntityType::COLLECTION, SubmeshHandle_t{ 0,0,0 }, children[i]->mName.C_Str(), localTransf };
            sceneGraph.insertNode(parentRE, rEnt);
            LoadNodeMeshes(scene, children[i]->mMeshes, children[i]->mNumMeshes, rEnt);
        }

        LoadNodeChildren(scene, children[i]->mChildren, children[i]->mNumChildren, rEnt);
        
    }
}

RenderableEntity RendererFrontend::LoadNodeMeshes(const aiScene* scene, unsigned int* meshesIndices, unsigned int numMeshes, RenderableEntity& parentRE, Transform_t localTransf)
{
    aiReturn success;

    SubmeshHandle_t submeshHandle{ 0,0,0,0 };
    RenderableEntity rEnt{};

    aiString basePath;
    scene->mMetaData->Get("basePath", basePath);

    float unitScaleFactor = 1.0f;
    if (scene->mMetaData->HasKey("UnitScaleFactor")) {
        scene->mMetaData->Get<float>("UnitScaleFactor", unitScaleFactor);
    }

    localTransf.scal.x /= unitScaleFactor;
    localTransf.scal.y /= unitScaleFactor;
    localTransf.scal.z /= unitScaleFactor;

    for (int j = 0; j < numMeshes; j++) {
        aiMesh* mesh = scene->mMeshes[meshesIndices[j]];

        //TODO: implement reading material info in material desc-------------------------
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        std::uint16_t matDescId = materials.size() - scene->mNumMaterials + mesh->mMaterialIndex;
        MaterialDesc& matDesc = materials.at(matDescId);
        MaterialCacheHandle_t initMatCacheHnd{ 0, NO_UPDATE_MAT};

        if (matDesc.shaderType == PShaderID::UNDEFINED) {
            wchar_t tmpString[128];

            aiColor3D diffuse(0.0f, 0.0f, 0.0f);
            if (AI_SUCCESS != material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse)) {
                Logger::AddLog(Logger::Channel::ERR, "%s\n", importer.GetErrorString());
            }

            matDesc.shaderType = PShaderID::STANDARD;

            aiString diffTexName;
            int count = material->GetTextureCount(aiTextureType_DIFFUSE);
            if (count > 0) {
                success = material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), diffTexName);
                if (!(aiReturn_SUCCESS == success)) {
                    Logger::AddLog(Logger::Channel::ERR, "%s\n", importer.GetErrorString());
                }
                else {
                    aiString albedoPath = aiString(basePath);
                    albedoPath.Append("/");
                    albedoPath.Append(diffTexName.C_Str());
                    mbstowcs(tmpString, albedoPath.C_Str(), 128);
                    wcscpy(matDesc.albedoPath, tmpString);
                    //wcscpy(matDesc.albedoPath, L"");
                    // L"./shaders/assets/Human/Textures/Head/JPG/Colour_8k.jpg";
                }
            }

            aiString normTexName;
            count = material->GetTextureCount(aiTextureType_NORMALS);
            if (count > 0) {
                success = material->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), normTexName);
                if (!(aiReturn_SUCCESS == success)) {
                    Logger::AddLog(Logger::Channel::ERR, "%s\n", importer.GetErrorString());
                }
                else {
                    aiString NormalMapPath = aiString(basePath);
                    NormalMapPath.Append("/");
                    NormalMapPath.Append(normTexName.C_Str());
                    mbstowcs(tmpString, NormalMapPath.C_Str(), 128);
                    wcscpy(matDesc.normalPath, tmpString);
                    //wcscpy(matDesc.normalPath, L"");
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

            aiString roughTexName;
            count = material->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS);
            if (count > 0) {
                success = material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE_ROUGHNESS, 0), roughTexName);
                if (!(aiReturn_SUCCESS == success)) {
                    Logger::AddLog(Logger::Channel::ERR, "%s\n", importer.GetErrorString());
                }
                else {
                    aiString roughnessMapPath = aiString(basePath);
                    roughnessMapPath.Append("/");
                    roughnessMapPath.Append(roughTexName.C_Str());
                    mbstowcs(tmpString, roughnessMapPath.C_Str(), 128);
                    wcscpy(matDesc.smoothnessMap, tmpString);
                    //wcscpy(matDesc.smoothnessMap, L"");
                    matDesc.unionTags |= SMOOTHNESS_IS_MAP;
                }
            }
            else {
                float roughness = 0.0f;
                success = material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
                if (aiReturn_SUCCESS == success) {
                    //TODO:: do correct conversion roughness surface
                    matDesc.smoothnessValue = 1.0f - roughness;
                }
                else {
                    matDesc.smoothnessValue = 0.5f;
                }
            }

            aiString metTexName;
            count = material->GetTextureCount(aiTextureType_METALNESS);
            if (count > 0) {
                success = material->Get(AI_MATKEY_TEXTURE(aiTextureType_METALNESS, 0), metTexName);
                if (!(aiReturn_SUCCESS == success)) {
                    Logger::AddLog(Logger::Channel::ERR, "%s\n", importer.GetErrorString());
                }
                else {
                    aiString metalnessMapPath = aiString(basePath);
                    metalnessMapPath.Append("/");
                    metalnessMapPath.Append(metTexName.C_Str());
                    mbstowcs(tmpString, metalnessMapPath.C_Str(), 128);
                    wcscpy(matDesc.metalnessMap, tmpString);
                    //wcscpy(matDesc.metalnessMap, L"");
                    matDesc.unionTags |= METALNESS_IS_MAP;
                }
            }
            else {
                float metalness = 0.0f;
                success = material->Get(AI_MATKEY_METALLIC_FACTOR, metalness);
                if (aiReturn_SUCCESS == success) {
                    matDesc.metalnessValue = metalness;
                }
                else {
                    matDesc.metalnessValue = 0.0f;
                }
            }

            initMatCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS | IS_FIRST_MAT_ALLOC;
        }
        //---------------------------------------------------

        submeshHandle.matDescIdx = matDescId;
        submeshHandle.numVertices = mesh->mNumVertices;
        submeshHandle.baseVertex = staticSceneVertexData.size();

        //TODO: hmmm? evaluate should if i reserve in advance? when?
        staticSceneVertexData.reserve((size_t)submeshHandle.numVertices + (size_t)submeshHandle.baseVertex);
        bool hasTangents = mesh->HasTangentsAndBitangents();
        bool hasTexCoords = mesh->HasTextureCoords(0);

        for (int i = 0; i < submeshHandle.numVertices; i++)
        {
            aiVector3D* vert = &(mesh->mVertices[i]);
            aiVector3D* normal = &(mesh->mNormals[i]);
            aiVector3D* textureCoord = &mesh->mTextureCoords[0][i];
            aiVector3D* tangent = &mesh->mTangents[i];


            staticSceneVertexData.push_back(Vertex(
                vert->x, vert->y, vert->z,
                hasTexCoords ? textureCoord->x : 0.0f, hasTexCoords ? textureCoord->y : 0.0f,
                normal->x, normal->y, normal->z,
                hasTangents ? tangent->x : 0.0f, hasTangents ? tangent->y : 0.0f, hasTangents ? tangent->z : 0.0f));
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

        rEnt = RenderableEntity{ hash_str_uint32(std::to_string(submeshHandle.baseVertex)), EntityType::MESH, submeshHandle, mesh->mName.C_Str(), localTransf };
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

void RendererFrontend::ParseSceneGraph(const Node<RenderableEntity>* node, const dx::XMMATRIX& parentTransf, std::vector<SubmeshInfo>& submeshesInView, std::vector<LightInfo>& lightsInView) {
    dx::XMMATRIX newTransf = mult(node->value.localWorldTransf, parentTransf);
    if (node->value.tag == EntityType::LIGHT) {
        LightInfo& lightInfo = sceneLights.at(node->value.lightHnd.index);
        lightInfo.finalWorldTransf = newTransf;
        lightsInView.push_back(lightInfo);
    }
    else {
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
    }
    
    for (Node<RenderableEntity>* n : node->children) {
        ParseSceneGraph(n, newTransf, submeshesInView, lightsInView);
    }
    
}

ViewDesc RendererFrontend::ComputeView(const Camera& cam)
{
    std::vector<SubmeshInfo> submeshesInView;
    submeshesInView.reserve(sceneMeshes.size());
    
    std::vector<LightInfo> lightsInView;
    
    Node<RenderableEntity>* root = sceneGraph.rootNode;

    ParseSceneGraph(root, dx::XMMatrixIdentity(), submeshesInView, lightsInView);

    std::uint8_t numDirLights = 0;
    std::uint8_t numPointLights = 0;
    std::uint8_t numSpotLights = 0;

    for (const LightInfo& lightInfo : lightsInView) {
        switch (lightInfo.tag)
        {
            case LightType::DIRECTIONAL:
            {
                numDirLights += 1;
                break;
            }
            
            case LightType::POINT:
            {
                numPointLights += 1;
                break;
            }

            case LightType::SPOT:
            {
                numSpotLights += 1;
                break;
            }

        }
    }

    //TODO: does move do what I was expecting?
    return ViewDesc{std::move(submeshesInView), std::move(lightsInView), numDirLights, numPointLights, numSpotLights, cam};
}
