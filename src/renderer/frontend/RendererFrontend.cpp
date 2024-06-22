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


namespace zorya
{
    namespace dx = DirectX;

    Renderer_Frontend rf;

    Transform_t build_transform(aiMatrix4x4 assimp_transform)
    {
        aiVector3D scal;
        aiVector3D pos;
        aiVector3D rot;
        assimp_transform.Decompose(scal, rot, pos);

        return Transform_t{ dx::XMFLOAT3{pos.x, pos.y, pos.z}, dx::XMFLOAT3{rot.x, rot.y, rot.z}, dx::XMFLOAT3{scal.x, scal.y, scal.z} };
    }

    Renderer_Frontend::Renderer_Frontend() : m_scene_graph(Renderable_Entity{ 0,Entity_Type::COLLECTION, {Submesh_Handle_t{0,0,0}}, "scene", IDENTITY_TRANSFORM }) {}

    //TODO: move somewhere else and use other hashing function; this is temporary
    inline uint32_t hash_str_uint32(const std::string& str)
    {

        uint32_t hash = 0x811c9dc5;
        uint32_t prime = 0x1000193;

        for (int i = 0; i < str.size(); ++i)
        {
            uint8_t value = str[i];
            hash = hash ^ value;
            hash *= prime;
        }

        return hash;

    }

    float computeMaxDistance(float const_attenuation, float linear_attenuation, float quadratic_attenuation)
    {
        float radius = sqrt(1.0f / quadratic_attenuation);
        float light_power = 1.0f;
        constexpr float threshold = 0.001f;
        float max_dist = radius * (sqrt(light_power / threshold) - 1.0f);
        assert(max_dist > 0.0f);

        return max_dist;
    }

    Renderable_Entity Renderer_Frontend::load_model_from_file(const std::string& filename, bool optimize_graph, bool force_flatten_scene)
    {

        m_importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, 65535);
        m_importer.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, 65535);

        const aiScene* scene = m_importer.ReadFile(filename,
            aiProcess_FindInvalidData |
            aiProcess_GenSmoothNormals |
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_CalcTangentSpace |
            aiProcess_GenBoundingBoxes |
            (optimize_graph ? aiProcess_OptimizeGraph : 0) |
            (force_flatten_scene ? aiProcess_PreTransformVertices : 0) |
            aiProcess_ConvertToLeftHanded
        );

        Renderable_Entity root_entity{ 0, Entity_Type::UNDEFINED, 0, 0, 0 };

        if (scene == nullptr)
        {
            Logger::add_log(Logger::Channel::ERR, "%s\n", m_importer.GetErrorString());
            return root_entity;
        }

        aiNode* model_root_node = scene->mRootNode;

        m_materials.resize(max(m_materials.size(), abs((int)(m_materials.size() + scene->mNumMaterials - m_freed_scene_material_indices.size()))));
        size_t sep_index = filename.find_last_of("/");
        aiString base_path = aiString(filename.substr(0, sep_index));
        scene->mMetaData->Add("basePath", base_path);

        const char* name = scene->GetShortFilename(filename.c_str());
        scene->mMetaData->Add("modelName", name);

        time_t timestamp = time(NULL);
        int rand_noise = rand();

        Renderable_Entity renderable_entity{ hash_str_uint32(std::string(name).append(std::to_string(rand_noise).append(std::to_string(timestamp)))), Entity_Type::COLLECTION, {Submesh_Handle_t{ 0,0,0 }}, name, IDENTITY_TRANSFORM };
        m_scene_graph.insertNode(Renderable_Entity{ 0 }, renderable_entity);

        load_node_children(scene, model_root_node->mChildren, model_root_node->mNumChildren, renderable_entity);

        Logger::add_log(Logger::Channel::TRACE, "Imported model %s\n", scene->GetShortFilename(filename.c_str()));

        return root_entity;
    }

    Renderable_Entity Renderer_Frontend::add_light(const Renderable_Entity* attach_to, dx::XMVECTOR direction, float shadow_map_near_plane, float shadow_map_far_plane)
    {
        Light_Handle_t hnd_light;
        hnd_light.tag = Light_Type::DIRECTIONAL;

        if (m_freed_scene_light_indices.size() > 0)
        {
            hnd_light.index = m_freed_scene_light_indices.back();

            Light_Info& light_info = m_scene_lights.at(hnd_light.index);
            light_info.tag = Light_Type::DIRECTIONAL;
            //TODO: define near/far plane computation based on some parameter thats still not defined
            light_info.dir_light = Directional_Light{ {}, shadow_map_near_plane, shadow_map_far_plane };
            dx::XMStoreFloat4(&light_info.dir_light.dir, direction);
            light_info.final_world_transform = dx::XMMatrixIdentity();

            m_freed_scene_light_indices.pop_back();
        }
        else
        {
            hnd_light.index = m_scene_lights.size();
            {
                Light_Info& light_info = m_scene_lights.emplace_back(Light_Info{ Light_Type::DIRECTIONAL, {Directional_Light{{}, shadow_map_near_plane, shadow_map_far_plane}}, dx::XMMatrixIdentity() });
                dx::XMStoreFloat4(&light_info.dir_light.dir, direction);
            }
        }

        //TODO:decide what to hash for id
        Renderable_Entity renderable_entity;
        renderable_entity.ID = hash_str_uint32(std::string("dir_light_") + std::to_string(m_scene_lights.size()));
        renderable_entity.entity_name = "Dir Light";
        renderable_entity.hnd_light = hnd_light;
        renderable_entity.tag = Entity_Type::LIGHT;
        renderable_entity.local_world_transf = IDENTITY_TRANSFORM;

        if (attach_to == nullptr)
        {
            m_scene_graph.insertNode(Renderable_Entity{ 0 }, renderable_entity);
            return renderable_entity;
        }
        else
        {
            //TODO: fix whatever this is
            assert(false);
            return renderable_entity;
        }

    }

    Renderable_Entity Renderer_Frontend::add_light(const Renderable_Entity* attach_to, dx::XMVECTOR direction, dx::XMVECTOR position, float cutoff_angle)
    {
        Light_Handle_t hnd_light;
        hnd_light.tag = Light_Type::SPOT;

        if (m_freed_scene_light_indices.size() > 0)
        {
            hnd_light.index = m_freed_scene_light_indices.back();

            Light_Info& light_info = m_scene_lights.at(hnd_light.index);
            light_info.tag = Light_Type::SPOT;
            //TODO: define near/far plane computation based on some parameter thats still not defined
            light_info.spot_light = Spot_Light{ {}, {}, std::cos(cutoff_angle), 1.0f, 20.0f };
            dx::XMStoreFloat4(&light_info.spot_light.direction, direction);
            dx::XMStoreFloat4(&light_info.spot_light.pos_world_space, position);
            light_info.final_world_transform = dx::XMMatrixIdentity();

            m_freed_scene_light_indices.pop_back();
        }
        else
        {
            hnd_light.index = m_scene_lights.size();

            Light_Info& light_info = m_scene_lights.emplace_back();
            light_info.tag = Light_Type::SPOT;
            //TODO: define near/far plane computation based on some parameter thats still not defined
            light_info.spot_light = Spot_Light{ {}, {}, std::cos(cutoff_angle), 1.0f, 20.0f };
            dx::XMStoreFloat4(&light_info.spot_light.direction, direction);
            dx::XMStoreFloat4(&light_info.spot_light.pos_world_space, position);
            light_info.final_world_transform = dx::XMMatrixIdentity();
        }

        //TODO:decide what to hash for id
        Renderable_Entity renderable_entity;
        renderable_entity.ID = hash_str_uint32(std::string("spot_light_") + std::to_string(m_scene_lights.size()));
        renderable_entity.entity_name = "Spot Light";
        renderable_entity.hnd_light = hnd_light;
        renderable_entity.tag = Entity_Type::LIGHT;
        renderable_entity.local_world_transf = IDENTITY_TRANSFORM;

        if (attach_to == nullptr)
        {
            m_scene_graph.insertNode(Renderable_Entity{ 0 }, renderable_entity);
            return renderable_entity;
        }
        else
        {
            //TODO: fix whatever this is
            assert(false);
            return renderable_entity;
        }

    }

    Renderable_Entity Renderer_Frontend::add_light(const Renderable_Entity* attachTo, dx::XMVECTOR position, float constant, float linear, float quadratic)
    {
        Light_Handle_t hnd_light;
        hnd_light.tag = Light_Type::POINT;

        float max_light_dist = computeMaxDistance(constant, linear, quadratic);

        if (m_freed_scene_light_indices.size() > 0)
        {
            hnd_light.index = m_freed_scene_light_indices.back();

            Light_Info& light_info = m_scene_lights.at(hnd_light.index);
            light_info.tag = Light_Type::POINT;
            light_info.point_light = Point_Light{ {}, constant, linear, quadratic, 1.0f, 1.0f + max_light_dist };
            dx::XMStoreFloat4(&light_info.point_light.pos_world_space, position);

            light_info.final_world_transform = dx::XMMatrixIdentity();

            m_freed_scene_light_indices.pop_back();
        }
        else
        {
            hnd_light.index = m_scene_lights.size();

            Light_Info& light_info = m_scene_lights.emplace_back();
            light_info.tag = Light_Type::POINT;
            light_info.point_light = Point_Light{ {}, constant, linear, quadratic, 1.0f, 1.0f + max_light_dist };
            dx::XMStoreFloat4(&light_info.point_light.pos_world_space, position);
            light_info.final_world_transform = dx::XMMatrixIdentity();
        }

        //TODO:decide what to hash for id
        Renderable_Entity renderable_entity;
        renderable_entity.ID = hash_str_uint32(std::string("point_light_") + std::to_string(m_scene_lights.size()));
        renderable_entity.entity_name = "Point Light";
        renderable_entity.hnd_light = hnd_light;
        renderable_entity.tag = Entity_Type::LIGHT;
        renderable_entity.local_world_transf = IDENTITY_TRANSFORM;

        if (attachTo == nullptr)
        {
            m_scene_graph.insertNode(Renderable_Entity{ 0 }, renderable_entity);
            return renderable_entity;
        }
        else
        {
            //TODO: fix whatever this is
            assert(false);
            return renderable_entity;
        }
    }


    int Renderer_Frontend::remove_entity(Renderable_Entity& entity_to_remove)
    {
        std::vector<Generic_Entity_Handle> handles_to_delete_data = m_scene_graph.remove_node(entity_to_remove);
        int num_mesh_entities_removed = 0, num_light_entities_removed = 0;

        for (Generic_Entity_Handle& hnd_entity : handles_to_delete_data)
        {
            switch (hnd_entity.tag)
            {

            case Entity_Type::LIGHT:
                m_freed_scene_light_indices.push_back(hnd_entity.hnd_light.index);
                num_light_entities_removed += 1;
                break;

            case Entity_Type::MESH:
                //TODO: fix this; dont use for, find a way to directly index 
                for (int i = 0; i < m_scene_meshes.size(); i++)
                {
                    Submesh_Info& submesh_info = m_scene_meshes.at(i);
                    if (submesh_info.hnd_submesh.base_vertex == hnd_entity.hnd_submesh.base_vertex)
                    {
                        m_freed_scene_mesh_indices.push_back(i);
                        m_freed_scene_material_indices.push_back(hnd_entity.hnd_submesh.material_desc_id);
                        static_cast<Reflection_Container<Standard_Material_Desc>*>(m_materials.at(hnd_entity.hnd_submesh.material_desc_id))->reflected_struct.shader_type = PShader_ID::UNDEFINED;

                        resource_cache.dealloc_material(submesh_info.hnd_material_cache);
                        buffer_cache.dealloc_static_geom(submesh_info.hnd_buffer_cache);

                        m_freed_scene_static_vertex_data_fragments.emplace_back(Fragment_Info{ hnd_entity.hnd_submesh.base_vertex, hnd_entity.hnd_submesh.num_vertices });
                        m_freed_scene_static_index_data_fragments.emplace_back(Fragment_Info{ hnd_entity.hnd_submesh.base_index, hnd_entity.hnd_submesh.num_indices });

                        num_mesh_entities_removed += 1;

                        break;
                    }
                }
                break;

            default:
                break;
            }
        }

        return num_mesh_entities_removed + num_light_entities_removed;
    }


    void Renderer_Frontend::load_node_children(const aiScene* scene, aiNode** children, unsigned int num_children, Renderable_Entity& parente_renderable_entity)
    {
        for (int i = 0; i < num_children; i++)
        {
            Transform_t local_transform = build_transform(children[i]->mTransformation);

            Renderable_Entity renderable_entity;
            if (children[i]->mNumMeshes == 1)
            {
                renderable_entity = load_node_meshes(scene, children[i]->mMeshes, children[i]->mNumMeshes, parente_renderable_entity, local_transform);
            }
            else
            {
                time_t timestamp = time(NULL);
                int rand_noise = rand();
                renderable_entity = Renderable_Entity{ hash_str_uint32(std::string(children[i]->mName.C_Str()).append(std::to_string(rand_noise).append(std::to_string(timestamp)))), Entity_Type::COLLECTION, { Submesh_Handle_t{ 0,0,0 } }, children[i]->mName.C_Str(), local_transform };
                m_scene_graph.insertNode(parente_renderable_entity, renderable_entity);
                load_node_meshes(scene, children[i]->mMeshes, children[i]->mNumMeshes, renderable_entity);
            }

            load_node_children(scene, children[i]->mChildren, children[i]->mNumChildren, renderable_entity);

        }
    }

    Renderable_Entity Renderer_Frontend::load_node_meshes(const aiScene* scene, unsigned int* meshes_indices, unsigned int num_meshes, Renderable_Entity& parent_renderable_entity, Transform_t local_transform)
    {
        aiReturn success;

        Submesh_Handle_t hnd_submesh{ 0,0,0,0 };
        Renderable_Entity renderable_entity{};

        aiString base_path;
        scene->mMetaData->Get("basePath", base_path);

        float unit_scale_factor = 1.0f;
        if (scene->mMetaData->HasKey("UnitScaleFactor"))
        {
            scene->mMetaData->Get<float>("UnitScaleFactor", unit_scale_factor);
        }

        local_transform.scal.x /= unit_scale_factor;
        local_transform.scal.y /= unit_scale_factor;
        local_transform.scal.z /= unit_scale_factor;

        //TODO: remove; just  for development
        local_transform.scal.x = 10.0f;
        local_transform.scal.y = 10.0f;
        local_transform.scal.z = 10.0f;
        local_transform.pos.y = -2.2f;

        size_t num_char_converted = 0;
        for (int j = 0; j < num_meshes; j++)
        {
            aiMesh* mesh = scene->mMeshes[meshes_indices[j]];

            //TODO: implement reading material info in material desc-------------------------
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            uint16_t mat_desc_id = 0;
            if (m_freed_scene_material_indices.size() > 0)
            {
                mat_desc_id = m_freed_scene_material_indices.back();
                m_freed_scene_material_indices.pop_back();
            }
            else
            {
                mat_desc_id = m_materials.size() - scene->mNumMaterials + mesh->mMaterialIndex;
            }

            //ReflectionBase* matDesc = materials.at(matDescId);
            Reflection_Container<Standard_Material_Desc>* material_desc = new Reflection_Container<Standard_Material_Desc>();
            m_materials.at(mat_desc_id) = material_desc;
            auto& material_params = material_desc->reflected_struct;
            material_params.shader_type = PShader_ID::UNDEFINED;

            Material_Cache_Handle_t hnd_init_mat_cache{ 0, NO_UPDATE_MAT };

            if (material_params.shader_type == PShader_ID::UNDEFINED)
            {

                material_params.sss_model.mean_free_path_distance = 0.01f;
                material_params.sss_model.mean_free_path_color = dx::XMFLOAT4(3.68f, 1.37f, 0.68f, 0.0f);
                material_params.sss_model.subsurface_albedo = dx::XMFLOAT4(0.436f, 0.015688f, 0.131f, 0.0f);
                material_params.sss_model.scale = 1.0f;
                material_params.sss_model.num_samples = 10;
                material_params.selected_sss_model = SSS_MODEL::NONE;

                wchar_t tmp_string[128];

                aiColor3D diffuse(0.0f, 0.0f, 0.0f);
                if (AI_SUCCESS != material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse))
                {
                    Logger::add_log(Logger::Channel::ERR, "%s\n", m_importer.GetErrorString());
                }

                material_params.shader_type = PShader_ID::STANDARD;

                aiString diffuse_tex_name;
                int count = material->GetTextureCount(aiTextureType_DIFFUSE);
                if (count > 0)
                {
                    success = material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), diffuse_tex_name);
                    if (!(aiReturn_SUCCESS == success))
                    {
                        Logger::add_log(Logger::Channel::ERR, "%s\n", m_importer.GetErrorString());
                    }
                    else
                    {
                        aiString albedo_path = aiString(base_path);
                        albedo_path.Append("/");
                        albedo_path.Append(diffuse_tex_name.C_Str());
                        mbstowcs_s(&num_char_converted, tmp_string, albedo_path.C_Str(), 128);
                        wcscpy_s(material_params.albedo_path, tmp_string);

                        // L"./shaders/assets/Human/Textures/Head/JPG/Colour_8k.jpg";
                    }
                }
                //wcscpy(matDesc.albedoPath, L"./shaders/assets/Human/Textures/Head/JPG/Colour_8k.jpg");//TODO: REMOVE AFTER TESTING
                //wcscpy(matDesc.albedoPath, L"./shaders/assets/Human/Textures/Head/Bake/Bake_4096_point.png");
                //wcscpy(material_params.albedo_path, L"");

                aiString normal_tex_name;
                count = material->GetTextureCount(aiTextureType_NORMALS);
                if (count > 0)
                {
                    success = material->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), normal_tex_name);
                    if (!(aiReturn_SUCCESS == success))
                    {
                        Logger::add_log(Logger::Channel::ERR, "%s\n", m_importer.GetErrorString());
                    }
                    else
                    {
                        aiString normal_tex_path = aiString(base_path);
                        normal_tex_path.Append("/");
                        normal_tex_path.Append(normal_tex_name.C_Str());
                        mbstowcs_s(&num_char_converted, tmp_string, normal_tex_path.C_Str(), 128);
                        wcscpy(material_params.normal_path, tmp_string);
                        // L"./shaders/assets/Human/Textures/Head/JPG/Normal Map_SubDivision_1.jpg";
                    }
                }
                //wcscpy(matDesc.normalPath, L"./shaders/assets/Human/Textures/Head/JPG/Normal Map_SubDivision_1.jpg");//TODO: REMOVE AFTER TESTING
                //wcscpy(material_params.normal_path, L"");

                aiColor4D color;
                success = material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
                if (aiReturn_SUCCESS == success)
                {
                    material_params.base_color = dx::XMFLOAT4(color.r, color.g, color.b, color.a);
                }
                else
                {
                    material_params.base_color = dx::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
                }
                material_params.base_color = dx::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);//TODO: REMOVE AFTER TESTING

                aiString roughness_tex_name;
                count = material->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS);
                if (count > 0)
                {
                    success = material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE_ROUGHNESS, 0), roughness_tex_name);
                    if (!(aiReturn_SUCCESS == success))
                    {
                        Logger::add_log(Logger::Channel::ERR, "%s\n", m_importer.GetErrorString());
                    }
                    else
                    {
                        aiString roughness_tex_path = aiString(base_path);
                        roughness_tex_path.Append("/");
                        roughness_tex_path.Append(roughness_tex_name.C_Str());
                        mbstowcs_s(&num_char_converted, tmp_string, roughness_tex_path.C_Str(), 128);
                        wcscpy_s(material_params.smoothness_map, tmp_string);
                        //wcscpy(matDesc.smoothnessMap, L"");
                        material_params.union_tags |= SMOOTHNESS_IS_MAP;
                    }
                }
                else
                {
                    float roughness = 0.0f;
                    success = material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
                    if (aiReturn_SUCCESS == success)
                    {
                        //TODO:: do correct conversion roughness surface
                        material_params.smoothness_value = 1.0f - roughness;
                    }
                    else
                    {
                        material_params.smoothness_value = 0.5f;
                    }
                }

                material_params.smoothness_value = 0.30f; //TODO: REMOVE AFTER TESTING

                aiString metalness_tex_name;
                count = material->GetTextureCount(aiTextureType_METALNESS);
                if (count > 0)
                {
                    success = material->Get(AI_MATKEY_TEXTURE(aiTextureType_METALNESS, 0), metalness_tex_name);
                    if (!(aiReturn_SUCCESS == success))
                    {
                        Logger::add_log(Logger::Channel::ERR, "%s\n", m_importer.GetErrorString());
                    }
                    else
                    {
                        aiString metalness_tex_path = aiString(base_path);
                        metalness_tex_path.Append("/");
                        metalness_tex_path.Append(metalness_tex_name.C_Str());
                        mbstowcs_s(&num_char_converted, tmp_string, metalness_tex_path.C_Str(), 128);
                        wcscpy_s(material_params.metalness_map, tmp_string);
                        //wcscpy(matDesc.metalnessMap, L"");
                        material_params.union_tags |= METALNESS_IS_MAP;
                    }
                }
                else
                {
                    float metalness = 0.0f;
                    success = material->Get(AI_MATKEY_METALLIC_FACTOR, metalness);
                    if (aiReturn_SUCCESS == success)
                    {
                        material_params.metalness_value = metalness;
                    }
                    else
                    {
                        material_params.metalness_value = 0.0f;
                    }
                }

                hnd_init_mat_cache.is_cached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS | IS_FIRST_MAT_ALLOC;
            }
            //---------------------------------------------------

            hnd_submesh.material_desc_id = mat_desc_id;

            //Vertices--------------------
            bool found_free_fragment = false;

            Fragment_Info* fragment_found = nullptr;

            for (Fragment_Info& freed_scene_vertex_fragment_info : m_freed_scene_static_vertex_data_fragments)
            {
                if (mesh->mNumVertices <= freed_scene_vertex_fragment_info.length)
                {
                    hnd_submesh.num_vertices = mesh->mNumVertices;
                    hnd_submesh.base_vertex = freed_scene_vertex_fragment_info.index;

                    fragment_found = &freed_scene_vertex_fragment_info;

                    //Fixing up remaining fragment after new reservation
                    uint32_t remaining_fragment_length = freed_scene_vertex_fragment_info.length - mesh->mNumVertices;
                    if (remaining_fragment_length == 0)
                    {
                        std::swap(freed_scene_vertex_fragment_info, m_freed_scene_static_vertex_data_fragments.back());
                        m_freed_scene_static_vertex_data_fragments.pop_back();
                    }
                    else
                    {
                        freed_scene_vertex_fragment_info.index += (freed_scene_vertex_fragment_info.length - remaining_fragment_length);
                        freed_scene_vertex_fragment_info.length = remaining_fragment_length;
                    }

                    found_free_fragment = true;
                    break;
                }
            }

            if (!found_free_fragment)
            {
                hnd_submesh.num_vertices = mesh->mNumVertices;
                hnd_submesh.base_vertex = m_static_scene_vertex_data.size();

                //TODO: hmmm? evaluate should if i reserve in advance? when?
                m_static_scene_vertex_data.reserve((size_t)hnd_submesh.num_vertices + (size_t)hnd_submesh.base_vertex);
            }

            bool has_tangents = mesh->HasTangentsAndBitangents();
            bool has_tex_coords = mesh->HasTextureCoords(0);

            for (int i = 0; i < hnd_submesh.num_vertices; i++)
            {
                aiVector3D* vert = &(mesh->mVertices[i]);
                aiVector3D* normal = &(mesh->mNormals[i]);
                aiVector3D* texture_coord = &mesh->mTextureCoords[0][i];
                aiVector3D* tangent = &mesh->mTangents[i];

                if (found_free_fragment)
                {
                    m_static_scene_vertex_data.at(hnd_submesh.base_vertex + i) = Vertex(
                        vert->x, vert->y, vert->z,
                        has_tex_coords ? texture_coord->x : 0.0f, has_tex_coords ? texture_coord->y : 0.0f,
                        normal->x, normal->y, normal->z,
                        has_tangents ? tangent->x : 0.0f, has_tangents ? tangent->y : 0.0f, has_tangents ? tangent->z : 0.0f);
                }
                else
                {
                    m_static_scene_vertex_data.push_back(Vertex(
                        vert->x, vert->y, vert->z,
                        has_tex_coords ? texture_coord->x : 0.0f, has_tex_coords ? texture_coord->y : 0.0f,
                        normal->x, normal->y, normal->z,
                        has_tangents ? tangent->x : 0.0f, has_tangents ? tangent->y : 0.0f, has_tangents ? tangent->z : 0.0f));
                }

            }


            //Indices--------------
            found_free_fragment = false;
            int num_faces = mesh->mNumFaces;

            for (Fragment_Info& freed_scene_index_fragment_info : m_freed_scene_static_index_data_fragments)
            {
                if (num_faces * 3 <= freed_scene_index_fragment_info.length)
                {
                    hnd_submesh.num_indices = num_faces * 3;
                    hnd_submesh.base_index = freed_scene_index_fragment_info.index;

                    //Fixing up remaining fragment after new reservation
                    uint32_t remaining_fragment_length = freed_scene_index_fragment_info.length - (num_faces * 3);
                    if (remaining_fragment_length == 0)
                    {
                        std::swap(freed_scene_index_fragment_info, m_freed_scene_static_index_data_fragments.back());
                        m_freed_scene_static_index_data_fragments.pop_back();
                    }
                    else
                    {
                        freed_scene_index_fragment_info.index += (freed_scene_index_fragment_info.length - remaining_fragment_length);
                        freed_scene_index_fragment_info.length = remaining_fragment_length;
                    }

                    found_free_fragment = true;
                    break;
                }
            }


            if (!found_free_fragment)
            {
                hnd_submesh.num_indices = num_faces * 3;
                hnd_submesh.base_index = m_static_scene_index_data.size();

                m_static_scene_index_data.reserve((size_t)hnd_submesh.base_index + (size_t)hnd_submesh.num_indices);
            }

            int indices_count = 0;

            for (int i = 0; i < num_faces; i++)
            {
                aiFace* face = &(mesh->mFaces[i]);

                for (int j = 0; j < face->mNumIndices; j++)
                {
                    if (found_free_fragment)
                    {
                        m_static_scene_index_data.at(hnd_submesh.base_index + indices_count) = face->mIndices[j];
                    }
                    else
                    {
                        m_static_scene_index_data.push_back(face->mIndices[j]);
                    }

                    indices_count += 1;
                }
            }

            if (m_freed_scene_mesh_indices.size() > 0)
            {
                int free_mesh_index = m_freed_scene_mesh_indices.back();

                Submesh_Info& submesh_info = m_scene_meshes.at(free_mesh_index);
                submesh_info.hnd_submesh = hnd_submesh;
                submesh_info.hnd_buffer_cache = Buffer_Cache_Handle_t{ 0,0 };
                submesh_info.hnd_material_cache = hnd_init_mat_cache;
                submesh_info.final_world_transform = dx::XMMatrixIdentity();

                m_freed_scene_mesh_indices.pop_back();
            }
            else
            {
                //TODO: maybe implement move for submeshHandle?
                m_scene_meshes.emplace_back(Submesh_Info{ hnd_submesh, Buffer_Cache_Handle_t{0,0}, hnd_init_mat_cache, dx::XMMatrixIdentity() });
            }

            time_t tm = time(NULL);
            renderable_entity = Renderable_Entity{ hash_str_uint32(std::string(mesh->mName.C_Str()).append(std::to_string(tm))), Entity_Type::MESH, {hnd_submesh}, mesh->mName.C_Str(), local_transform };
            m_scene_graph.insertNode(parent_renderable_entity, renderable_entity);
        }

        return renderable_entity;
    }

    Submesh_Info* Renderer_Frontend::find_submesh_info(Submesh_Handle_t hnd_submesh)
    {
        for (Submesh_Info& submesh_info : m_scene_meshes)
        {
            //TODO: make better comparison
            if (submesh_info.hnd_submesh.base_vertex == hnd_submesh.base_vertex && submesh_info.hnd_submesh.num_vertices == hnd_submesh.num_vertices)
            {
                return &submesh_info;
            }
        }
        return nullptr;
    }

    void Renderer_Frontend::parse_scene_graph(const Node<Renderable_Entity>* node, const dx::XMMATRIX& parent_transform, std::vector<Submesh_Info>& submeshes_in_view, std::vector<Light_Info>& lights_in_view)
    {
        dx::XMMATRIX new_transform = mult(node->value.local_world_transf, parent_transform);

        if (node->value.tag == Entity_Type::LIGHT)
        {
            Light_Info& light_info = m_scene_lights.at(node->value.hnd_light.index);
            light_info.final_world_transform = new_transform;
            lights_in_view.push_back(light_info);
        }
        else
        {
            if (node->value.hnd_submesh.num_vertices > 0)
            {
                Submesh_Info* submesh_info = find_submesh_info(node->value.hnd_submesh);
                //TODO: implement frustum culling

                bool cached = submesh_info->hnd_buffer_cache.is_cached;
                if (!cached)
                {
                    submesh_info->hnd_buffer_cache = buffer_cache.alloc_static_geom(submesh_info->hnd_submesh, m_static_scene_index_data.data() + submesh_info->hnd_submesh.base_index, m_static_scene_vertex_data.data() + submesh_info->hnd_submesh.base_vertex);
                }

                cached = (submesh_info->hnd_material_cache.is_cached & (UPDATE_MAT_MAPS | UPDATE_MAT_PRMS)) == 0;
                if (!cached)
                {
                    submesh_info->hnd_material_cache = resource_cache.alloc_material(m_materials.at(submesh_info->hnd_submesh.material_desc_id), submesh_info->hnd_material_cache);
                }
                submesh_info->final_world_transform = new_transform;
                submeshes_in_view.push_back(*submesh_info);
            }
        }

        for (Node<Renderable_Entity>* node_renderable_entity : node->children)
        {
            parse_scene_graph(node_renderable_entity, new_transform, submeshes_in_view, lights_in_view);
        }

    }

    View_Desc Renderer_Frontend::compute_view(const Camera& cam)
    {
        std::vector<Submesh_Info> submeshes_in_view;
        submeshes_in_view.reserve(m_scene_meshes.size());

        std::vector<Light_Info> lights_in_view;

        Node<Renderable_Entity>* scene_graph_root = m_scene_graph.root_node;

        parse_scene_graph(scene_graph_root, dx::XMMatrixIdentity(), submeshes_in_view, lights_in_view);

        uint8_t num_dir_lights = 0;
        uint8_t num_point_lights = 0;
        uint8_t num_spot_lights = 0;

        for (const Light_Info& light_info : lights_in_view)
        {
            switch (light_info.tag)
            {
            case Light_Type::DIRECTIONAL:
            {
                num_dir_lights += 1;
                break;
            }

            case Light_Type::POINT:
            {
                num_point_lights += 1;
                break;
            }

            case Light_Type::SPOT:
            {
                num_spot_lights += 1;
                break;
            }

            }
        }

        //TODO: does move do what I was expecting?
        return View_Desc{ std::move(submeshes_in_view), std::move(lights_in_view), num_dir_lights, num_point_lights, num_spot_lights, cam };
    }

}