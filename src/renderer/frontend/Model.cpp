#include "Model.h"
#include "SceneManager.h"
#include "editor/Logger.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "renderer/backend/PipelineStateObject.h"
#include "renderer/backend/rhi/RenderDevice.h"
#include "renderer/frontend/Shader.h"

#include "editor/AssetRegistry.h"

#include "utils/Hash.h"

#include <cstdint>
#include <vector>
#include <ctime>
#include <utility>
#include <algorithm>

namespace zorya
{

    std::vector<Vertex> cube_vertices =
    {
        // Front Face
        Vertex(-1.0f, -1.0f, -1.0f, 0.0f, 1.0f,-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f),
        Vertex(-1.0f,  1.0f, -1.0f, 0.0f, 0.0f,-1.0f,  1.0f, -1.0f, 1.0f, 1.0f, 1.0f),
        Vertex(1.0f,  1.0f, -1.0f, 1.0f, 0.0f, 1.0f,  1.0f, -1.0f, 1.0f, 1.0f, 1.0f),
        Vertex(1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f),

        // Back Face
        Vertex(-1.0f, -1.0f, 1.0f, 1.0f, 1.0f,-1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f),
        Vertex(1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f),
        Vertex(1.0f,  1.0f, 1.0f, 0.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f, 1.0f),
        Vertex(-1.0f,  1.0f, 1.0f, 1.0f, 0.0f,-1.0f,  1.0f, 1.0f, 1.0f, 1.0f, 1.0f),

        // Top Face
        Vertex(-1.0f, 1.0f, -1.0f, 0.0f, 1.0f,-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f),
        Vertex(-1.0f, 1.0f,  1.0f, 0.0f, 0.0f,-1.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f),
        Vertex(1.0f, 1.0f,  1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f),
        Vertex(1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f),

        // Bottom Face
        Vertex(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f,-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f),
        Vertex(1.0f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f),
        Vertex(1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 1.0f, -1.0f,  1.0f, 1.0f, 1.0f, 1.0f),
        Vertex(-1.0f, -1.0f,  1.0f, 1.0f, 0.0f,-1.0f, -1.0f,  1.0f, 1.0f, 1.0f, 1.0f),

        // Left Face
        Vertex(-1.0f, -1.0f,  1.0f, 0.0f, 1.0f,-1.0f, -1.0f,  1.0f, 1.0f, 1.0f, 1.0f),
        Vertex(-1.0f,  1.0f,  1.0f, 0.0f, 0.0f,-1.0f,  1.0f,  1.0f, 1.0f, 1.0f, 1.0f),
        Vertex(-1.0f,  1.0f, -1.0f, 1.0f, 0.0f,-1.0f,  1.0f, -1.0f, 1.0f, 1.0f, 1.0f),
        Vertex(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f,-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f),

        // Right Face
        Vertex(1.0f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f),
        Vertex(1.0f,  1.0f, -1.0f, 0.0f, 0.0f, 1.0f,  1.0f, -1.0f, 1.0f, 1.0f, 1.0f),
        Vertex(1.0f,  1.0f,  1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  1.0f, 1.0f, 1.0f, 1.0f),
        Vertex(1.0f, -1.0f,  1.0f, 1.0f, 1.0f, 1.0f, -1.0f,  1.0f, 1.0f, 1.0f, 1.0f),
    };

    std::vector<std::uint16_t> cube_indices =
    {
        // Front Face
        0,  1,  2,
        0,  2,  3,

        // Back Face
        4,  5,  6,
        4,  6,  7,

        // Top Face
        8,  9, 10,
        8, 10, 11,

        // Bottom Face
        12, 13, 14,
        12, 14, 15,

        // Left Face
        16, 17, 18,
        16, 18, 19,

        // Right Face
        20, 21, 22,
        20, 22, 23
    };


    std::vector<Simple_Vertex> quad_vertices =
    {
        Simple_Vertex(-1.0f, -1.0f, 0.0f),
        Simple_Vertex(-1.0f, 1.0f, 0.0f),
        Simple_Vertex(1.0f, 1.0f, 0.0f),
        Simple_Vertex(1.0f, -1.0f, 0.0f)
    };

    Model3D* Model3D::create(const Asset_Import_Config* asset_imp_conf)
    {
        assert(asset_imp_conf != nullptr);

        Model3D* model = new Model3D(asset_imp_conf);

        return model;
    }

    bool sort_compare_submesh_collection(Submesh_Collection& i, Submesh_Collection& j) { return (i.depth < j.depth); }

    Model3D::Model3D(const Asset_Import_Config* asset_imp_conf) : Asset(asset_imp_conf)
    {
        bool optimize_graph = false;
        bool force_flatten_scene = false;

        //rf.m_importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, 65535);
        //rf.m_importer.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, 65535);

        const aiScene* scene = scene_manager.m_importer.ReadFile(asset_imp_conf->asset_filepath,
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

        if (scene == nullptr)
        {
            Logger::add_log(Logger::Channel::ERR, "%s\n", scene_manager.m_importer.GetErrorString());
        }
        else
        {
            aiNode* model_root_node = scene->mRootNode;

            std::string filename(asset_imp_conf->asset_filepath);
            const char* model_name = scene->GetShortFilename(filename.c_str());
            model_root_node->mName.Set(model_name);

            global_material = scene_manager.hnd_default_material;

            load_node_children(scene, &model_root_node, 1, 0);

            //TODO: reactivate sort if first submesh collection is not the root
            //std::sort(submesh_collections.begin(), submesh_collections.end(), sort_compare_submesh_collection);

            Logger::add_log(Logger::Channel::TRACE, "Imported model %s\n", model_name);
        }
    }

    std::vector<int> Model3D::load_node_children(const aiScene* scene, aiNode** children, unsigned int num_children, unsigned int depth)
    {
        std::vector<int> collection_indices{};

        for (int i = 0; i < num_children; i++)
        {
            Submesh_Collection submesh_collection;
            submesh_collection.depth = depth;
            submesh_collection.local_transform = build_transform(children[i]->mTransformation);

            if (children[i]->mNumMeshes > 0)
            {
                submesh_collection.submesh_indices = load_node_meshes(scene, children[i]->mMeshes, children[i]->mNumMeshes, IDENTITY_TRANSFORM);
            }
        
            collection_names.emplace_back(std::string(children[i]->mName.C_Str()));
            submesh_collections.push_back(submesh_collection);
            int collection_index = submesh_collections.size() - 1;
            
            collection_indices.push_back(submesh_collections.size() - 1);

            auto children_collection_indices = load_node_children(scene, children[i]->mChildren, children[i]->mNumChildren, depth + 1);
            submesh_collections.at(collection_index).collection_indices = children_collection_indices;

        }

        return collection_indices;
    }

    std::vector<int> Model3D::load_node_meshes(const aiScene* scene, unsigned int* meshes_indices, unsigned int num_meshes, Transform_t local_transform)
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

        //local_transform.scal.x /= unit_scale_factor;
        //local_transform.scal.y /= unit_scale_factor;
        //local_transform.scal.z /= unit_scale_factor;

        //TODO: remove; just  for development
        //local_transform.scal.x = 10.0f;
        //local_transform.scal.y = 10.0f;
        //local_transform.scal.z = 10.0f;
        //local_transform.pos.y = -2.2f;

        std::vector<int> submesh_indices;

        size_t num_char_converted = 0;
        for (int j = 0; j < num_meshes; j++)
        {
            aiMesh* mesh = scene->mMeshes[meshes_indices[j]];

            //TODO: implement reading material info in material desc-------------------------
            //aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            //
            //Material_Cache_Handle_t material_hnd{ 0, IS_FIRST_MAT_ALLOC };
            //material_hnd = resource_cache.alloc_material(material_hnd);
            //Material& mat = resource_cache.m_material_cache.at(material_hnd.index);

            //Filesystem_Node& node = asset_registry.current_node->value;
            //zassert(node.m_type == Filesystem_Node_Type::FOLDER);

            //char material_filepath[MAX_PATH];
            //ZeroMemory(material_filepath, MAX_PATH);

            //strncat_s(material_filepath, asset_registry.current_node->value.m_folder_path, MAX_PATH);
            //strncat_s(material_filepath, "\\", MAX_PATH);
            //int index_last_char_base_path = strnlen(material_filepath, MAX_PATH);

            //int file_creation_attempts = 0;
            //char tentative_filename[MAX_PATH];
            //ZeroMemory(tentative_filename, MAX_PATH);

            //FILE* file, * metafile = nullptr;
            //bool is_found_free_filename = false;

            //while (!is_found_free_filename)
            //{
            //    sprintf(tentative_filename, "%s%d.mat", material->GetName().C_Str(), file_creation_attempts);

            //    material_filepath[index_last_char_base_path] = 0;
            //    strncat_s(material_filepath, tentative_filename, MAX_PATH);

            //    file = fopen(material_filepath, "r");

            //    if (file != nullptr)
            //    {
            //        fclose(file);
            //        file_creation_attempts += 1;
            //    } else
            //    {
            //        file = fopen(material_filepath, "wb");
            //        mat.m_file_path = std::string(material_filepath);

            //        strncat_s(material_filepath, ".metafile", MAX_PATH);
            //        metafile = fopen(material_filepath, "wb");

            //        is_found_free_filename = true;
            //    }
            //}

            //if (file != nullptr) fclose(file);
            //if (metafile != nullptr) fclose(metafile);

            //int res = mat.serialize();

            ////TODO: dont use global allocator
            //Asset_Import_Config* asset_imp_config = new Asset_Import_Config;
            //asset_imp_config->asset_type = mat.asset_type;
            //asset_imp_config->guid = mat.m_guid;

            //const char* asset_filepath = mat.m_file_path.c_str();
            //size_t path_buff_size = strnlen_s(asset_filepath, MAX_PATH) + 1;
            //asset_imp_config->asset_filepath = (char*)malloc(path_buff_size);
            //strncpy_s(asset_imp_config->asset_filepath, path_buff_size, asset_filepath, path_buff_size - 1);

            //res = Asset_Import_Config::serialize(asset_imp_config);

            //if (res == 0) OutputDebugString("Failed serialization of material\n");
            //else
            //{
            //    //strncat_s(tentative_filename, ".metafile", strnlen(".metafile", MAX_PATH));
            //    asset_registry.current_node->children.emplace_back(Filesystem_Node(Filesystem_Node_Type::FILE, &mat, asset_imp_config, tentative_filename), asset_registry.current_node);
            //}

            //Constant_Buffer_Data* mat_prms = nullptr;
            //for (auto& resource : mat.resources)
            //{
            //    if(strncmp(resource.m_name, "_matPrms", strnlen("_matPrms", 255)) == 0) mat_prms = resource.m_parameters;
            //}
            //
            //if (mat_prms != nullptr)
            //{
            //    aiColor3D diffuse(0.9f, 0.9f, 0.9f);
            //    if (AI_SUCCESS != material->Get(AI_MATKEY_BASE_COLOR, diffuse))
            //    {
            //        Logger::add_log(Logger::Channel::ERR, "%s\n", scene_manager.m_importer.GetErrorString());
            //    } 

            //    float base_color[] = { diffuse.r, diffuse.g, diffuse.b, 1.0f };
            //    set_constant_buff_var(mat_prms, "_baseColor", base_color, sizeof(base_color));

            //    float roughness = 0.5f;
            //    if (AI_SUCCESS != material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness))
            //    {
            //        Logger::add_log(Logger::Channel::ERR, "%s\n", scene_manager.m_importer.GetErrorString());
            //    } 

            //    set_constant_buff_var(mat_prms, "_cb_roughness", &roughness, sizeof(roughness));

            //    float metalness = 0.0f;
            //    if (AI_SUCCESS != material->Get(AI_MATKEY_METALLIC_FACTOR, metalness))
            //    {
            //        Logger::add_log(Logger::Channel::ERR, "%s\n", scene_manager.m_importer.GetErrorString());
            //    } 

            //    set_constant_buff_var(mat_prms, "_cb_metallic", &metalness, sizeof(metalness));

            //    mat.serialize();
            //}

            //uint16_t mat_desc_id = 0;
            //if (m_freed_scene_material_indices.size() > 0)
            //{
            //    mat_desc_id = m_freed_scene_material_indices.back();
            //    m_freed_scene_material_indices.pop_back();
            //}
            //else
            //{
            //    mat_desc_id = m_materials.size() - scene->mNumMaterials + mesh->mMaterialIndex;
            //}

            //ReflectionBase* matDesc = materials.at(matDescId);
            //Reflection_Container<Standard_Material_Desc>* material_desc = new Reflection_Container<Standard_Material_Desc>();
            //m_materials.at(mat_desc_id) = material_desc;
            //auto& material_params = material_desc->reflected_struct;
            //material_params.shader_type = PShader_ID::UNDEFINED;

            //Material_Cache_Handle_t hnd_init_mat_cache = rf.hnd_default_material;//{ 0, NO_UPDATE_MAT };

            //if (material_params.shader_type == PShader_ID::UNDEFINED)
            //{

            //    material_params.sss_model.mean_free_path_distance = 0.01f;
            //    material_params.sss_model.mean_free_path_color = dx::XMFLOAT4(3.68f, 1.37f, 0.68f, 0.0f);
            //    material_params.sss_model.subsurface_albedo = dx::XMFLOAT4(0.436f, 0.015688f, 0.131f, 0.0f);
            //    material_params.sss_model.scale = 1.0f;
            //    material_params.sss_model.num_samples = 10;
            //    material_params.selected_sss_model = SSS_MODEL::NONE;

            //    wchar_t tmp_string[128];

            //    aiColor3D diffuse(0.0f, 0.0f, 0.0f);
            //    if (AI_SUCCESS != material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse))
            //    {
            //        Logger::add_log(Logger::Channel::ERR, "%s\n", m_importer.GetErrorString());
            //    }

            //    material_params.shader_type = PShader_ID::STANDARD;

            //    aiString diffuse_tex_name;
            //    int count = material->GetTextureCount(aiTextureType_DIFFUSE);
            //    if (count > 0)
            //    {
            //        success = material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), diffuse_tex_name);
            //        if (!(aiReturn_SUCCESS == success))
            //        {
            //            Logger::add_log(Logger::Channel::ERR, "%s\n", m_importer.GetErrorString());
            //        }
            //        else
            //        {
            //            aiString albedo_path = aiString(base_path);
            //            albedo_path.Append("/");
            //            albedo_path.Append(diffuse_tex_name.C_Str());
            //            mbstowcs_s(&num_char_converted, tmp_string, albedo_path.C_Str(), 128);
            //            wcscpy_s(material_params.albedo_path, tmp_string);
            //            material_params.albedo_path[0] = 0;

            //            // L"./shaders/assets/Human/Textures/Head/JPG/Colour_8k.jpg";
            //        }
            //    }
            //    //wcscpy(matDesc.albedoPath, L"./shaders/assets/Human/Textures/Head/JPG/Colour_8k.jpg");//TODO: REMOVE AFTER TESTING
            //    //wcscpy(matDesc.albedoPath, L"./shaders/assets/Human/Textures/Head/Bake/Bake_4096_point.png");
            //    //wcscpy(material_params.albedo_path, L"");

            //    aiString normal_tex_name;
            //    count = material->GetTextureCount(aiTextureType_NORMALS);
            //    if (count > 0)
            //    {
            //        success = material->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), normal_tex_name);
            //        if (!(aiReturn_SUCCESS == success))
            //        {
            //            Logger::add_log(Logger::Channel::ERR, "%s\n", m_importer.GetErrorString());
            //        }
            //        else
            //        {
            //            aiString normal_tex_path = aiString(base_path);
            //            normal_tex_path.Append("/");
            //            normal_tex_path.Append(normal_tex_name.C_Str());
            //            mbstowcs_s(&num_char_converted, tmp_string, normal_tex_path.C_Str(), 128);
            //            wcscpy(material_params.normal_path, tmp_string);
            //            // L"./shaders/assets/Human/Textures/Head/JPG/Normal Map_SubDivision_1.jpg";
            //        }
            //    }
            //    //wcscpy(matDesc.normalPath, L"./shaders/assets/Human/Textures/Head/JPG/Normal Map_SubDivision_1.jpg");//TODO: REMOVE AFTER TESTING
            //    //wcscpy(material_params.normal_path, L"");

            //    aiColor4D color;
            //    success = material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
            //    if (aiReturn_SUCCESS == success)
            //    {
            //        material_params.base_color = dx::XMFLOAT4(color.r, color.g, color.b, color.a);
            //    }
            //    else
            //    {
            //        material_params.base_color = dx::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
            //    }
            //    material_params.base_color = dx::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);//TODO: REMOVE AFTER TESTING

            //    aiString roughness_tex_name;
            //    count = material->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS);
            //    if (count > 0)
            //    {
            //        success = material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE_ROUGHNESS, 0), roughness_tex_name);
            //        if (!(aiReturn_SUCCESS == success))
            //        {
            //            Logger::add_log(Logger::Channel::ERR, "%s\n", m_importer.GetErrorString());
            //        }
            //        else
            //        {
            //            aiString roughness_tex_path = aiString(base_path);
            //            roughness_tex_path.Append("/");
            //            roughness_tex_path.Append(roughness_tex_name.C_Str());
            //            mbstowcs_s(&num_char_converted, tmp_string, roughness_tex_path.C_Str(), 128);
            //            wcscpy_s(material_params.smoothness_map, tmp_string);
            //            //wcscpy(matDesc.smoothnessMap, L"");
            //            material_params.union_tags |= SMOOTHNESS_IS_MAP;
            //        }
            //    }
            //    else
            //    {
            //        float roughness = 0.0f;
            //        success = material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
            //        if (aiReturn_SUCCESS == success)
            //        {
            //            //TODO:: do correct conversion roughness surface
            //            material_params.smoothness_value = 1.0f - roughness;
            //        }
            //        else
            //        {
            //            material_params.smoothness_value = 0.5f;
            //        }
            //    }

                //material_params.smoothness_value = 0.30f; //TODO: REMOVE AFTER TESTING

                //aiString metalness_tex_name;
                //count = material->GetTextureCount(aiTextureType_METALNESS);
                //if (count > 0)
            //    {
            //        success = material->Get(AI_MATKEY_TEXTURE(aiTextureType_METALNESS, 0), metalness_tex_name);
            //        if (!(aiReturn_SUCCESS == success))
            //        {
            //            Logger::add_log(Logger::Channel::ERR, "%s\n", m_importer.GetErrorString());
            //        }
            //        else
            //        {
            //            aiString metalness_tex_path = aiString(base_path);
            //            metalness_tex_path.Append("/");
            //            metalness_tex_path.Append(metalness_tex_name.C_Str());
            //            mbstowcs_s(&num_char_converted, tmp_string, metalness_tex_path.C_Str(), 128);
            //            wcscpy_s(material_params.metalness_map, tmp_string);
            //            //wcscpy(matDesc.metalnessMap, L"");
            //            material_params.union_tags |= METALNESS_IS_MAP;
            //        }
            //    }
            //    else
            //    {
            //        float metalness = 0.0f;
            //        success = material->Get(AI_MATKEY_METALLIC_FACTOR, metalness);
            //        if (aiReturn_SUCCESS == success)
            //        {
            //            material_params.metalness_value = metalness;
            //        }
            //        else
            //        {
            //            material_params.metalness_value = 0.0f;
            //        }
            //    }

            //    hnd_init_mat_cache.is_cached = UPDATE_MAT_MAPS | UPDATE_MAT_PRMS | IS_FIRST_MAT_ALLOC;
            //}

            //---------------------------------------------------

            //Material_Cache_Handle_t mat_options{ 0,IS_FIRST_MAT_ALLOC };
            //hnd_init_mat_cache = resource_cache.alloc_material(mat_options);

            //hnd_submesh.material_desc_id = mat_desc_id;

            //Vertices--------------------
            bool found_free_fragment = false;

            //Fragment_Info* fragment_found = nullptr;

            for (Fragment_Info& freed_scene_vertex_fragment_info : scene_manager.m_freed_scene_static_vertex_data_fragments)
            {
                if (mesh->mNumVertices <= freed_scene_vertex_fragment_info.length)
                {
                    hnd_submesh.num_vertices = mesh->mNumVertices;
                    hnd_submesh.base_vertex = freed_scene_vertex_fragment_info.index;

                    //fragment_found = &freed_scene_vertex_fragment_info;

                    //Fixing up remaining fragment after new reservation
                    uint32_t remaining_fragment_length = freed_scene_vertex_fragment_info.length - mesh->mNumVertices;
                    if (remaining_fragment_length == 0)
                    {
                        std::swap(freed_scene_vertex_fragment_info, scene_manager.m_freed_scene_static_vertex_data_fragments.back());
                        scene_manager.m_freed_scene_static_vertex_data_fragments.pop_back();
                    } else
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
                hnd_submesh.base_vertex = scene_manager.m_static_scene_vertex_data.size();

                //TODO: hmmm? evaluate should if i reserve in advance? when?
                scene_manager.m_static_scene_vertex_data.reserve((size_t)hnd_submesh.num_vertices + (size_t)hnd_submesh.base_vertex);
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
                    scene_manager.m_static_scene_vertex_data.at(hnd_submesh.base_vertex + i) = Vertex(
                        vert->x, vert->y, vert->z,
                        has_tex_coords ? texture_coord->x : 0.0f, has_tex_coords ? texture_coord->y : 0.0f,
                        normal->x, normal->y, normal->z,
                        has_tangents ? tangent->x : 0.0f, has_tangents ? tangent->y : 0.0f, has_tangents ? tangent->z : 0.0f);
                } else
                {
                    scene_manager.m_static_scene_vertex_data.push_back(Vertex(
                        vert->x, vert->y, vert->z,
                        has_tex_coords ? texture_coord->x : 0.0f, has_tex_coords ? texture_coord->y : 0.0f,
                        normal->x, normal->y, normal->z,
                        has_tangents ? tangent->x : 0.0f, has_tangents ? tangent->y : 0.0f, has_tangents ? tangent->z : 0.0f));
                }

            }


            //Indices--------------
            found_free_fragment = false;
            int num_faces = mesh->mNumFaces;

            for (Fragment_Info& freed_scene_index_fragment_info : scene_manager.m_freed_scene_static_index_data_fragments)
            {
                if (num_faces * 3 <= freed_scene_index_fragment_info.length)
                {
                    hnd_submesh.num_indices = num_faces * 3;
                    hnd_submesh.base_index = freed_scene_index_fragment_info.index;

                    //Fixing up remaining fragment after new reservation
                    uint32_t remaining_fragment_length = freed_scene_index_fragment_info.length - (num_faces * 3);
                    if (remaining_fragment_length == 0)
                    {
                        std::swap(freed_scene_index_fragment_info, scene_manager.m_freed_scene_static_index_data_fragments.back());
                        scene_manager.m_freed_scene_static_index_data_fragments.pop_back();
                    } else
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
                hnd_submesh.base_index = scene_manager.m_static_scene_index_data.size();

                scene_manager.m_static_scene_index_data.reserve((size_t)hnd_submesh.base_index + (size_t)hnd_submesh.num_indices);
            }

            int indices_count = 0;

            for (int i = 0; i < num_faces; i++)
            {
                aiFace* face = &(mesh->mFaces[i]);

                for (int j = 0; j < face->mNumIndices; j++)
                {
                    if (found_free_fragment)
                    {
                        scene_manager.m_static_scene_index_data.at(hnd_submesh.base_index + indices_count) = face->mIndices[j];
                    } else
                    {
                        scene_manager.m_static_scene_index_data.push_back(face->mIndices[j]);
                    }

                    indices_count += 1;
                }
            }

            //if (rf.m_freed_scene_mesh_indices.size() > 0)
            //{
            //    int free_mesh_index = rf.m_freed_scene_mesh_indices.back();

            //    Submesh_Info& submesh_info = rf.m_scene_meshes.at(free_mesh_index);
            //    submesh_info.hnd_submesh = hnd_submesh;
            //    submesh_info.hnd_buffer_cache = Buffer_Cache_Handle_t{ 0,0 };
            //    submesh_info.hnd_material_cache = hnd_init_mat_cache;
            //    submesh_info.final_world_transform = dx::XMMatrixIdentity();

            //    rf.m_freed_scene_mesh_indices.pop_back();
            //} else
            //{
            //    //TODO: maybe implement move for submeshHandle?
            //    rf.m_scene_meshes.emplace_back(Submesh_Info{ hnd_submesh, Buffer_Cache_Handle_t{0,0}, hnd_init_mat_cache, /*hnd_init_mat_cache,*/ dx::XMMatrixIdentity() });
            //}

            //TODO: for materials use handle; also allow material creation from import
            submesh_materials.push_back(nullptr);
            submeshes.push_back(hnd_submesh);
            


            //submesh_psos.push_back(pso_desc);

            int submesh_index = submeshes.size() - 1;
            submesh_indices.push_back(submesh_index);

            submesh_names.emplace_back(std::string(mesh->mName.C_Str()));

        }

        return submesh_indices;
    }

}