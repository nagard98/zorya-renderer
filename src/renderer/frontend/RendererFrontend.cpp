#include "RendererFrontend.h"
#include "Model.h"
#include "Material.h"
#include "Camera.h"
#include "SceneGraph.h"
#include "Shader.h"
#include "Lights.h"
#include "Transform.h"

#include "reflection/Reflection.h"

#include "renderer/backend/ResourceCache.h"
#include "renderer/backend/BufferCache.h"

#include "editor/Logger.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "DirectXMath.h"

#include <cassert>
#include <iostream>
#include <winerror.h>
#include <cstdlib>
#include <algorithm>


namespace dx = DirectX;

RendererFrontend rf;

Transform_t buildTransform(aiMatrix4x4 assTransf) {
    aiVector3D scal;
    aiVector3D pos;
    aiVector3D rot;
    assTransf.Decompose(scal, rot, pos);

    return Transform_t{ dx::XMFLOAT3{pos.x, pos.y, pos.z}, dx::XMFLOAT3{rot.x, rot.y, rot.z}, dx::XMFLOAT3{scal.x, scal.y, scal.z} };
}

RendererFrontend::RendererFrontend() : sceneGraph(RenderableEntity{ 0,EntityType::COLLECTION, {SubmeshHandle_t{0,0,0}}, "scene", IDENTITY_TRANSFORM }) {}
   
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

float computeMaxDistance(float constAtt, float linAtt, float quadAtt) 
{
    float radius = sqrt(1.0f / quadAtt);
    float lightPow = 1.0f;
    constexpr float threshold = 0.001f;
    float maxDist = radius * (sqrt(lightPow / threshold) - 1.0f);
    assert(maxDist > 0.0f);

    return maxDist;
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

    RenderableEntity rootEnt{ 0, EntityType::UNDEFINED, 0, 0, 0 };

    if (scene == nullptr)
    {
        Logger::AddLog(Logger::Channel::ERR, "%s\n", importer.GetErrorString());
        return rootEnt;
    }

    aiNode* rootNode = scene->mRootNode;

    materials.resize(max(materials.size(), materials.size() + scene->mNumMaterials - freedSceneMaterialIndices.size()));
    size_t sepIndex = filename.find_last_of("/");
    aiString basePath = aiString(filename.substr(0, sepIndex));
    scene->mMetaData->Add("basePath", basePath);

    const char* name = scene->GetShortFilename(filename.c_str());
    scene->mMetaData->Add("modelName", name);

    time_t timestamp = time(NULL);
    int randNoise = rand();

    RenderableEntity rEnt{ hash_str_uint32(std::string(name).append(std::to_string(randNoise).append(std::to_string(timestamp)))), EntityType::COLLECTION, {SubmeshHandle_t{ 0,0,0 }}, name, IDENTITY_TRANSFORM };
    sceneGraph.insertNode(RenderableEntity{ 0 }, rEnt);

    LoadNodeChildren(scene, rootNode->mChildren, rootNode->mNumChildren, rEnt);

    Logger::AddLog(Logger::Channel::TRACE, "Imported model %s\n", scene->GetShortFilename(filename.c_str()));

	return rootEnt;
}

RenderableEntity RendererFrontend::AddLight(const RenderableEntity* attachTo, dx::XMVECTOR direction, float shadowMapNearPlane, float shadowMapFarPlane)
{
        LightHandle_t lightHnd;
        lightHnd.tag = LightType::DIRECTIONAL;

        if (freedSceneLightIndices.size() > 0) {
            lightHnd.index = freedSceneLightIndices.back();

            LightInfo& lightInfo = sceneLights.at(lightHnd.index);
            lightInfo.tag = LightType::DIRECTIONAL;
            //TODO: define near/far plane computation based on some parameter thats still not defined
            lightInfo.dirLight = DirectionalLight{ {}, shadowMapNearPlane, shadowMapFarPlane };
            dx::XMStoreFloat4(&lightInfo.dirLight.dir, direction);
            lightInfo.finalWorldTransf = dx::XMMatrixIdentity();

            freedSceneLightIndices.pop_back();
        }
        else {
            lightHnd.index = sceneLights.size();
            {
                LightInfo& lightInfo = sceneLights.emplace_back(LightInfo{ LightType::DIRECTIONAL, {DirectionalLight{{}, shadowMapNearPlane, shadowMapFarPlane}}, dx::XMMatrixIdentity() });
                dx::XMStoreFloat4(&lightInfo.dirLight.dir, direction);
            }
        }

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
        //TODO: fix whatever this is
        assert(false);
        return rEnt;
    }

}

RenderableEntity RendererFrontend::AddLight(const RenderableEntity* attachTo, dx::XMVECTOR direction, dx::XMVECTOR position, float cutoffAngle)
{
    LightHandle_t lightHnd;
    lightHnd.tag = LightType::SPOT;
 
    if (freedSceneLightIndices.size() > 0) {
        lightHnd.index = freedSceneLightIndices.back();

        LightInfo& lightInfo = sceneLights.at(lightHnd.index);
        lightInfo.tag = LightType::SPOT;
        //TODO: define near/far plane computation based on some parameter thats still not defined
        lightInfo.spotLight = SpotLight{ {}, {}, std::cos(cutoffAngle), 1.0f, 20.0f };
        dx::XMStoreFloat4(&lightInfo.spotLight.direction, direction);
        dx::XMStoreFloat4(&lightInfo.spotLight.posWorldSpace, position);
        lightInfo.finalWorldTransf = dx::XMMatrixIdentity();

        freedSceneLightIndices.pop_back();
    }
    else {
        lightHnd.index = sceneLights.size();

        LightInfo& lightInfo = sceneLights.emplace_back();
        lightInfo.tag = LightType::SPOT;
        //TODO: define near/far plane computation based on some parameter thats still not defined
        lightInfo.spotLight = SpotLight{ {}, {}, std::cos(cutoffAngle), 1.0f, 20.0f };
        dx::XMStoreFloat4(&lightInfo.spotLight.direction, direction);
        dx::XMStoreFloat4(&lightInfo.spotLight.posWorldSpace, position);
        lightInfo.finalWorldTransf = dx::XMMatrixIdentity();
    }

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
        //TODO: fix whatever this is
        assert(false);
        return rEnt;
    }

}

RenderableEntity RendererFrontend::AddLight(const RenderableEntity* attachTo, dx::XMVECTOR position, float constant, float linear, float quadratic)
{
    LightHandle_t lightHnd;
    lightHnd.tag = LightType::POINT;

    float maxLightDist = computeMaxDistance(constant, linear, quadratic);

    if (freedSceneLightIndices.size() > 0) {
        lightHnd.index = freedSceneLightIndices.back();

        LightInfo& lightInfo = sceneLights.at(lightHnd.index);
        lightInfo.tag = LightType::POINT;
        lightInfo.pointLight = PointLight{ {}, constant, linear, quadratic, 1.0f, 1.0f + maxLightDist };
        dx::XMStoreFloat4(&lightInfo.pointLight.posWorldSpace, position);

        lightInfo.finalWorldTransf = dx::XMMatrixIdentity();

        freedSceneLightIndices.pop_back();
    }
    else {
        lightHnd.index = sceneLights.size();

        LightInfo& lightInfo = sceneLights.emplace_back();
        lightInfo.tag = LightType::POINT;
        lightInfo.pointLight = PointLight{ {}, constant, linear, quadratic, 1.0f, 1.0f + maxLightDist };
        dx::XMStoreFloat4(&lightInfo.pointLight.posWorldSpace, position);
        lightInfo.finalWorldTransf = dx::XMMatrixIdentity();
    }

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
        //TODO: fix whatever this is
        assert(false);
        return rEnt;
    }
}


int RendererFrontend::removeEntity(RenderableEntity& entityToRemove)
{
    std::vector<GenericEntityHandle> handlesToDeleteData = sceneGraph.removeNode(entityToRemove);
    int numMeshEntitiesRemoved = 0, numLightEntitiesRemoved = 0;

    for (GenericEntityHandle& entHnd : handlesToDeleteData) {
        switch (entHnd.tag) {

        case EntityType::LIGHT:
            freedSceneLightIndices.push_back(entHnd.lightHnd.index);
            numLightEntitiesRemoved += 1;
            break;

        case EntityType::MESH:
            //TODO: fix this; dont use for, find a way to directly index 
            for (int i = 0; i < sceneMeshes.size(); i++) {
                SubmeshInfo& submeshInfo = sceneMeshes.at(i);
                if (submeshInfo.submeshHnd.baseVertex == entHnd.submeshHnd.baseVertex) {
                    freedSceneMeshIndices.push_back(i);
                    freedSceneMaterialIndices.push_back(entHnd.submeshHnd.matDescIdx);
                    static_cast<ReflectionContainer<StandardMaterialDesc>*>(materials.at(entHnd.submeshHnd.matDescIdx))->reflectedStruct.shaderType = PShaderID::UNDEFINED;

                    resourceCache.DeallocMaterial(submeshInfo.matCacheHnd);
                    bufferCache.DeallocStaticGeom(submeshInfo.bufferHnd);

                    freedStaticSceneVertexDataFragments.emplace_back(FragmentInfo{ entHnd.submeshHnd.baseVertex, entHnd.submeshHnd.numVertices });
                    freedStaticSceneIndexDataFragments.emplace_back(FragmentInfo{ entHnd.submeshHnd.baseIndex, entHnd.submeshHnd.numIndexes });

                    numMeshEntitiesRemoved += 1;

                    break;
                }
            }
            break;

        default:
            break;
        }
    }

    return numMeshEntitiesRemoved + numLightEntitiesRemoved;
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
            time_t timestamp = time(NULL);
            int randNoise = rand();
            rEnt = RenderableEntity{ hash_str_uint32(std::string(children[i]->mName.C_Str()).append(std::to_string(randNoise).append(std::to_string(timestamp)))), EntityType::COLLECTION, { SubmeshHandle_t{ 0,0,0 } }, children[i]->mName.C_Str(), localTransf };
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

    //TODO: remove; just  for development
    //localTransf.scal.x = 10.0f;
    //localTransf.scal.y = 10.0f;
    //localTransf.scal.z = 10.0f;
    localTransf.pos.y = -2.2f;

    size_t numCharConverted = 0;
    for (int j = 0; j < numMeshes; j++) {
        aiMesh* mesh = scene->mMeshes[meshesIndices[j]];

        //TODO: implement reading material info in material desc-------------------------
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        std::uint16_t matDescId = 0;
        if (freedSceneMaterialIndices.size() > 0) {
            matDescId = freedSceneMaterialIndices.back();
            freedSceneMaterialIndices.pop_back();
        }
        else {
            matDescId = materials.size() - scene->mNumMaterials + mesh->mMaterialIndex;
        }

        //ReflectionBase* matDesc = materials.at(matDescId);
        ReflectionContainer<StandardMaterialDesc>* matDesc = new ReflectionContainer<StandardMaterialDesc>();
        materials.at(matDescId) = matDesc;
        auto& materialParams = matDesc->reflectedStruct;
        materialParams.shaderType = PShaderID::UNDEFINED;

        MaterialCacheHandle_t initMatCacheHnd{ 0, NO_UPDATE_MAT};

        if (materialParams.shaderType == PShaderID::UNDEFINED) {

            materialParams.meanFreePathDistance = 0.01f;
            materialParams.meanFreePathColor = dx::XMFLOAT4(3.68f, 1.37f, 0.68f, 1.0f);
            materialParams.subsurfaceAlbedo = dx::XMFLOAT4(0.436f, 0.015688f, 0.131f,1.0f);
            materialParams.scale = 5.0f;

            wchar_t tmpString[128];

            aiColor3D diffuse(0.0f, 0.0f, 0.0f);
            if (AI_SUCCESS != material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse)) {
                Logger::AddLog(Logger::Channel::ERR, "%s\n", importer.GetErrorString());
            }

            materialParams.shaderType = PShaderID::STANDARD;

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
                    mbstowcs_s(&numCharConverted, tmpString, albedoPath.C_Str(), 128);
                    wcscpy_s(materialParams.albedoPath, tmpString);

                    // L"./shaders/assets/Human/Textures/Head/JPG/Colour_8k.jpg";
                }
            }
            //wcscpy(matDesc.albedoPath, L"./shaders/assets/Human/Textures/Head/JPG/Colour_8k.jpg");//TODO: REMOVE AFTER TESTING
            //wcscpy(matDesc.albedoPath, L"./shaders/assets/Human/Textures/Head/Bake/Bake_4096_point.png");
            //wcscpy(matDesc.albedoPath, L"");

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
                    mbstowcs_s(&numCharConverted, tmpString, NormalMapPath.C_Str(), 128);
                    //wcscpy(matDesc.normalPath, tmpString);
                    // L"./shaders/assets/Human/Textures/Head/JPG/Normal Map_SubDivision_1.jpg";
                }
            }
            //wcscpy(matDesc.normalPath, L"./shaders/assets/Human/Textures/Head/JPG/Normal Map_SubDivision_1.jpg");//TODO: REMOVE AFTER TESTING
            //wcscpy(matDesc.normalPath, L"");

            aiColor4D col;
            success = material->Get(AI_MATKEY_COLOR_DIFFUSE, col);
            if (aiReturn_SUCCESS == success) {
                materialParams.baseColor = dx::XMFLOAT4(col.r, col.g, col.b, col.a);
            }
            else {
                materialParams.baseColor = dx::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
            }
            materialParams.baseColor = dx::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);//TODO: REMOVE AFTER TESTING

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
                    mbstowcs_s(&numCharConverted, tmpString, roughnessMapPath.C_Str(), 128);
                    wcscpy_s(materialParams.smoothnessMap, tmpString);
                    //wcscpy(matDesc.smoothnessMap, L"");
                    materialParams.unionTags |= SMOOTHNESS_IS_MAP;
                }
            }
            else {
                float roughness = 0.0f;
                success = material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
                if (aiReturn_SUCCESS == success) {
                    //TODO:: do correct conversion roughness surface
                    materialParams.smoothnessValue = 1.0f - roughness;
                }
                else {
                    materialParams.smoothnessValue = 0.5f;
                }
            }

            materialParams.smoothnessValue = 0.30f; //TODO: REMOVE AFTER TESTING

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
                    mbstowcs_s(&numCharConverted, tmpString, metalnessMapPath.C_Str(), 128);
                    wcscpy_s(materialParams.metalnessMap, tmpString);
                    //wcscpy(matDesc.metalnessMap, L"");
                    materialParams.unionTags |= METALNESS_IS_MAP;
                }
            }
            else {
                float metalness = 0.0f;
                success = material->Get(AI_MATKEY_METALLIC_FACTOR, metalness);
                if (aiReturn_SUCCESS == success) {
                    materialParams.metalnessValue = metalness;
                }
                else {
                    materialParams.metalnessValue = 0.0f;
                }
            }

            initMatCacheHnd.isCached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS | IS_FIRST_MAT_ALLOC;
        }
        //---------------------------------------------------

        submeshHandle.matDescIdx = matDescId;

        //Vertices--------------------
        bool foundFreeFragment = false;

        for (FragmentInfo& freedSceneVertexFragmentInfo : freedStaticSceneVertexDataFragments) {
            if (mesh->mNumVertices <= freedSceneVertexFragmentInfo.length) {
                submeshHandle.numVertices = mesh->mNumVertices;
                submeshHandle.baseVertex = freedSceneVertexFragmentInfo.index;

                //Fixing up remaining fragment after new reservation
                std::uint32_t remainingFragmentLength = mesh->mNumVertices - freedSceneVertexFragmentInfo.length;
                if (remainingFragmentLength == 0) {
                    std::swap(freedSceneVertexFragmentInfo, freedStaticSceneVertexDataFragments.back());
                    freedStaticSceneVertexDataFragments.pop_back();
                }
                else {
                     freedSceneVertexFragmentInfo.index += (freedSceneVertexFragmentInfo.length - remainingFragmentLength);
                     freedSceneVertexFragmentInfo.length = remainingFragmentLength;
                }

                foundFreeFragment = true;
                break;
            }
        }

        if (!foundFreeFragment) {
            submeshHandle.numVertices = mesh->mNumVertices;
            submeshHandle.baseVertex = staticSceneVertexData.size();

            //TODO: hmmm? evaluate should if i reserve in advance? when?
            staticSceneVertexData.reserve((size_t)submeshHandle.numVertices + (size_t)submeshHandle.baseVertex);
        }

        bool hasTangents = mesh->HasTangentsAndBitangents();
        bool hasTexCoords = mesh->HasTextureCoords(0);

        for (int i = 0; i < submeshHandle.numVertices; i++)
        {
            aiVector3D* vert = &(mesh->mVertices[i]);
            aiVector3D* normal = &(mesh->mNormals[i]);
            aiVector3D* textureCoord = &mesh->mTextureCoords[0][i];
            aiVector3D* tangent = &mesh->mTangents[i];

            if (foundFreeFragment) {
                staticSceneVertexData.at(submeshHandle.baseVertex + i) = Vertex(
                    vert->x, vert->y, vert->z,
                    hasTexCoords ? textureCoord->x : 0.0f, hasTexCoords ? textureCoord->y : 0.0f,
                    normal->x, normal->y, normal->z,
                    hasTangents ? tangent->x : 0.0f, hasTangents ? tangent->y : 0.0f, hasTangents ? tangent->z : 0.0f);
            }
            else {
                staticSceneVertexData.push_back(Vertex(
                    vert->x, vert->y, vert->z,
                    hasTexCoords ? textureCoord->x : 0.0f, hasTexCoords ? textureCoord->y : 0.0f,
                    normal->x, normal->y, normal->z,
                    hasTangents ? tangent->x : 0.0f, hasTangents ? tangent->y : 0.0f, hasTangents ? tangent->z : 0.0f));
            }

        }


        //Indices--------------
        foundFreeFragment = false;
        int numFaces = mesh->mNumFaces;

        for (FragmentInfo& freedSceneIndexFragmentInfo : freedStaticSceneIndexDataFragments) {
            if (numFaces * 3 <= freedSceneIndexFragmentInfo.length) {
                submeshHandle.numIndexes = numFaces * 3;
                submeshHandle.baseIndex = freedSceneIndexFragmentInfo.index;

                //Fixing up remaining fragment after new reservation
                std::uint32_t remainingFragmentLength = (numFaces * 3) - freedSceneIndexFragmentInfo.length;
                if (remainingFragmentLength == 0) {
                    std::swap(freedSceneIndexFragmentInfo, freedStaticSceneIndexDataFragments.back());
                    freedStaticSceneIndexDataFragments.pop_back();
                }
                else {
                    freedSceneIndexFragmentInfo.index += (freedSceneIndexFragmentInfo.length - remainingFragmentLength);
                    freedSceneIndexFragmentInfo.length = remainingFragmentLength;
                }

                foundFreeFragment = true;
                break;
            }
        }


        if (!foundFreeFragment) {
            submeshHandle.numIndexes = numFaces * 3;
            submeshHandle.baseIndex = staticSceneIndexData.size();

            staticSceneIndexData.reserve((size_t)submeshHandle.baseIndex + (size_t)submeshHandle.numIndexes);
        }

        int indicesCount = 0;

        for (int i = 0; i < numFaces; i++) {
            aiFace* face = &(mesh->mFaces[i]);

            for (int j = 0; j < face->mNumIndices; j++) {
                if (foundFreeFragment) {
                    staticSceneIndexData.at(submeshHandle.baseIndex + indicesCount) = face->mIndices[j];
                }
                else {
                    staticSceneIndexData.push_back(face->mIndices[j]);
                }

                indicesCount += 1;
            }
        }

        if (freedSceneMeshIndices.size() > 0) {
            int freeMeshIndex = freedSceneMeshIndices.back();
            SubmeshInfo& submeshInfo = sceneMeshes.at(freeMeshIndex);
            submeshInfo.submeshHnd = submeshHandle;
            submeshInfo.bufferHnd = BufferCacheHandle_t{ 0,0 };
            submeshInfo.matCacheHnd = initMatCacheHnd;
            submeshInfo.finalWorldTransf = dx::XMMatrixIdentity();

            freedSceneMeshIndices.pop_back();
        }
        else {
            //TODO: maybe implement move for submeshHandle?
            sceneMeshes.emplace_back(SubmeshInfo{ submeshHandle, BufferCacheHandle_t{0,0}, initMatCacheHnd, dx::XMMatrixIdentity() });
        }

        time_t tm = time(NULL);
        rEnt = RenderableEntity{ hash_str_uint32(std::string(mesh->mName.C_Str()).append(std::to_string(tm))), EntityType::MESH, {submeshHandle}, mesh->mName.C_Str(), localTransf };
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
