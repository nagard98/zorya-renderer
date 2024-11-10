#include "SceneManager.h"
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
#include "renderer/backend/rhi/RenderHardwareInterface.h"

#include "renderer/frontend/Transform.h"

#include "editor/Logger.h"

#include "utils/Hash.h"

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

    Scene_Manager scene_manager;

    Scene_Manager::Scene_Manager() : 
        m_scene_graph(Renderable_Entity{ 0, Entity_Type::COLLECTION, nullptr, "scene", IDENTITY_TRANSFORM }), 
        hnd_default_material(Material_Cache_Handle_t{ 0,IS_FIRST_MAT_ALLOC }) {}


    HRESULT Scene_Manager::init()
    {
        hnd_default_material = resource_cache.alloc_material(hnd_default_material);
        
        return S_OK;
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


    void Scene_Manager::add_light(Node<Renderable_Entity>* attach_to, dx::XMVECTOR direction, float shadow_map_near_plane, float shadow_map_far_plane)
    {
        assert(attach_to != nullptr);

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

        attach_to->children.push_back(new Node<Renderable_Entity>(renderable_entity));
    }

    void Scene_Manager::add_light(Node<Renderable_Entity>* attach_to, dx::XMVECTOR direction, dx::XMVECTOR position, float cutoff_angle)
    {
        assert(attach_to != nullptr);

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

        attach_to->children.push_back(new Node<Renderable_Entity>(renderable_entity));
    }

    void Scene_Manager::add_light(Node<Renderable_Entity>* attach_to, dx::XMVECTOR position, float constant, float linear, float quadratic)
    {
        assert(attach_to != nullptr);

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

        attach_to->children.push_back(new Node<Renderable_Entity>(renderable_entity));
    }

    void Scene_Manager::add_skylight(Node<Renderable_Entity>* attach_to)
    {
        assert(attach_to != nullptr);

        Light_Handle_t hnd_light;
        hnd_light.tag = Light_Type::SKYLIGHT;

        if (m_freed_scene_light_indices.size() > 0)
        {
            hnd_light.index = m_freed_scene_light_indices.back();

            Light_Info& light_info = m_scene_lights.at(hnd_light.index);
            light_info.tag = Light_Type::SKYLIGHT;
            light_info.sky_light.environment_texture = nullptr;
            light_info.final_world_transform = dx::XMMatrixIdentity();

            m_freed_scene_light_indices.pop_back();
        } else
        {
            hnd_light.index = m_scene_lights.size();

            Light_Info& light_info = m_scene_lights.emplace_back();
            light_info.tag = Light_Type::SKYLIGHT;
            light_info.sky_light.environment_texture = nullptr;
            light_info.final_world_transform = dx::XMMatrixIdentity();
        }

        //TODO:decide what to hash for id
        Renderable_Entity renderable_entity;
        renderable_entity.ID = hash_str_uint32(std::string("sky_light_") + std::to_string(m_scene_lights.size()));
        renderable_entity.entity_name = "Sky Light";
        renderable_entity.hnd_light = hnd_light;
        renderable_entity.tag = Entity_Type::LIGHT;
        renderable_entity.local_world_transf = IDENTITY_TRANSFORM;

        attach_to->children.push_back(new Node<Renderable_Entity>(renderable_entity));
    }

    u16 Scene_Manager::add_diffusion_profile(Diffusion_Profile_Handle hnd_profile)
    {
        for (size_t i = 0; i < scene_dprofiles.size(); i++)
        {
            auto hnd = scene_dprofiles[i];
            if (hnd.index == hnd_profile.index)
            {
                return i;
            }
        }

        scene_dprofiles.push_back(hnd_profile);

        return (u16)scene_dprofiles.size()-1;
    }

    int Scene_Manager::remove_node(Node<Renderable_Entity>* node_to_remove, Node<Renderable_Entity>* parent_of_node)
    {
        m_scene_graph.remove_node(node_to_remove, parent_of_node);
        
        return 0;
    }


    void Scene_Manager::parse_scene_graph(const Node<Renderable_Entity>* node, const dx::XMMATRIX& parent_transform, std::vector<Submesh_Render_Data>& submeshes_in_view, std::vector<Light_Info>& lights_in_view)
    {
        dx::XMMATRIX new_transform = mult(node->value.local_world_transf, parent_transform);

        switch (node->value.tag)
        {
        case zorya::Entity_Type::MESH:
        {
            Submesh_Desc* submesh_desc = node->value.submesh_desc;
            //TODO: implement frustum culling

            bool cached = submesh_desc->hnd_buffer_gpu_resource.is_cached;
            //TODO: move away allocation from here; this is a hot code path
            if (!cached)
            {
                submesh_desc->hnd_buffer_gpu_resource = buffer_cache.alloc_static_geom(submesh_desc->hnd_submesh, m_static_scene_index_data.data() + submesh_desc->hnd_submesh.base_index, m_static_scene_vertex_data.data() + submesh_desc->hnd_submesh.base_vertex);
            }

            submeshes_in_view.emplace_back(Submesh_Render_Data{ *submesh_desc, new_transform });
            break;
        }
        case zorya::Entity_Type::LIGHT:
        {
            Light_Info& light_info = m_scene_lights.at(node->value.hnd_light.index);
            light_info.final_world_transform = new_transform;
            lights_in_view.push_back(light_info);
            break;
        }
        default:
            break;
        }

        for (Node<Renderable_Entity>* node_renderable_entity : node->children)
        {
            parse_scene_graph(node_renderable_entity, new_transform, submeshes_in_view, lights_in_view);
        }

    }

    View_Desc Scene_Manager::compute_view(const Camera& cam)
    {
        View_Desc view_desc;
        view_desc.cam = cam;
        //std::vector<Submesh_Render_Data> submeshes_in_view;
        //TODO: fix reserved size
        view_desc.submeshes_info.reserve(255/*m_scene_meshes.size()*/);

        view_desc.sss_profiles = scene_dprofiles;

        Node<Renderable_Entity>* scene_graph_root = m_scene_graph.root_node;

        parse_scene_graph(scene_graph_root, dx::XMMatrixIdentity(), view_desc.submeshes_info, view_desc.lights_info);

        view_desc.num_dir_lights = 0;
        view_desc.num_point_lights = 0;
        view_desc.num_spot_lights = 0;

        for (const Light_Info& light_info : view_desc.lights_info)
        {
            switch (light_info.tag)
            {
            case Light_Type::DIRECTIONAL:
            {
                view_desc.num_dir_lights += 1;
                break;
            }

            case Light_Type::POINT:
            {
                view_desc.num_point_lights += 1;
                break;
            }

            case Light_Type::SPOT:
            {
                view_desc.num_spot_lights += 1;
                break;
            }
            
            case Light_Type::SKYLIGHT:
            {
                view_desc.skylight = light_info.sky_light;
                break;
            }

            default:
                zassert(false);

            }
        }

        std::sort(view_desc.lights_info.begin(), view_desc.lights_info.end(), [](Light_Info& a, Light_Info& b) { return (uint8_t)a.tag < (uint8_t)b.tag; });

        //TODO: does move do what I was expecting?
        //return View_Desc{ std::move(submeshes_in_view), std::move(contributing_lights), num_dir_lights, num_point_lights, num_spot_lights, cam };
        return view_desc;
    }

    Node<Renderable_Entity>* build_renderable_entity_tree(Node<Renderable_Entity>* attach_to, const Model3D* model, int current_mesh_index)
    {
        assert(model != nullptr);

        Node<Renderable_Entity>* new_node = nullptr;

        auto& current_collection = model->submesh_collections.at(current_mesh_index);
        
        if ((current_collection.collection_indices.size() != 0) || (current_collection.submesh_indices.size() != 0))
        {
            Renderable_Entity renderable_entity;
            renderable_entity.ID = rand();
            renderable_entity.tag = Entity_Type::COLLECTION;
            renderable_entity.local_world_transf = current_collection.local_transform;
            renderable_entity.entity_name = model->collection_names.at(current_mesh_index);

            auto collection_node = new Node<Renderable_Entity>(renderable_entity);
            attach_to->children.push_back(collection_node);

            for (int submesh_index : current_collection.submesh_indices)
            {
                //Mesh Node
                //TODO : use guid generation instead of rand
                renderable_entity.ID = rand();
                renderable_entity.tag = Entity_Type::MESH;
                renderable_entity.submesh_desc = new Submesh_Desc;
                renderable_entity.submesh_desc->hnd_submesh = model->submeshes.at(submesh_index);
                renderable_entity.entity_name = model->submesh_names.at(submesh_index);
                
                //const PSO_Desc& pso_desc = model->submesh_psos.at(submesh_index);
                //zassert(rhi.create_pso(&renderable_entity.submesh_desc->hnd_pso, pso_desc).value == S_OK);

                auto local_material = model->submesh_materials.at(submesh_index);
                renderable_entity.submesh_desc->hnd_material_cache = (local_material == nullptr ? model->global_material : local_material->material_hnd);

                renderable_entity.submesh_desc->hnd_buffer_gpu_resource = Buffer_Handle_t{ 0, false };

                renderable_entity.local_world_transf = IDENTITY_TRANSFORM;

                auto mesh_node = new Node<Renderable_Entity>(renderable_entity);
                collection_node->children.push_back(mesh_node);
            }

            for (int child_collection_index : current_collection.collection_indices)
            {
                build_renderable_entity_tree(collection_node, model, child_collection_index);
            }

            new_node = collection_node;
        }

        return new_node;
    }

}