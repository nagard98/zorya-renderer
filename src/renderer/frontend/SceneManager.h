#ifndef RENDERER_FRONTEND_H_
#define RENDERER_FRONTEND_H_

#include "renderer/backend/BufferCache.h"
#include "renderer/backend/ResourceCache.h"

#include "Model.h"
#include "Camera.h"
#include "SceneGraph.h"
#include "Lights.h"
#include "Transform.h"
#include "Material.h"
#include "DiffusionProfile.h"

#include <DirectXMath.h>

#include "assimp/scene.h"
#include "assimp/Importer.hpp"

#include <string>
#include <cstdint>

namespace zorya
{
	namespace dx = DirectX;

	//struct RenderableEntityHandle_t {
	//	uint32_t index;
	//	uint32_t numChildren;
	//	uint8_t isValid;
	//};

	//struct Transform_t {
	//	dx::XMFLOAT3 pos;
	//	dx::XMFLOAT3 rot;
	//	dx::XMFLOAT3 scal;
	//};
	//
	//constexpr Transform_t IDENTITY_TRANSFORM{ dx::XMFLOAT3(0.0f,0.0f,0.0f), dx::XMFLOAT3(0.0f,0.0f,0.0f), dx::XMFLOAT3(1.0f,1.0f,1.0f) };

	enum class Entity_Type : uint8_t
	{
		UNDEFINED,
		MESH,
		LIGHT,
		CAMERA,
		COLLECTION
	};

	struct Fragment_Info
	{
		uint32_t index;
		uint32_t length;
	};

	//struct Generic_Entity_Handle
	//{
	//	Entity_Type tag;
	//	union
	//	{
	//		Submesh_Handle_t hnd_submesh;
	//		Light_Handle_t hnd_light;
	//	};
	//};

	struct Submesh_Desc
	{
		Submesh_Handle_t hnd_submesh;
		Buffer_Handle_t hnd_buffer_gpu_resource;
		Material_Cache_Handle_t hnd_material_cache;
		//PSO_Handle hnd_pso;
	};

	struct Renderable_Entity
	{
		~Renderable_Entity()
		{		}

		bool operator==(const Renderable_Entity& r)
		{
			return ID == r.ID;
		}

		uint32_t ID;
		Entity_Type tag;
		union
		{
			Submesh_Desc* submesh_desc;
			Light_Handle_t hnd_light;
		};
		std::string entity_name;
		Transform_t local_world_transf;
		//Bound aabb;
	};

	struct Submesh_Render_Data
	{
		/*Submesh_Handle_t hnd_submesh;
		Buffer_Handle_t hnd_buffer_gpu_resource;
		Material_Cache_Handle_t hnd_material_cache;*/
		Submesh_Desc submesh_desc;
		dx::XMMATRIX final_world_transform;
	};

	struct Light_Info
	{
		Light_Type tag;
		union
		{
			Directional_Light dir_light;
			Point_Light point_light;
			Spot_Light spot_light;
			Sky_Light sky_light;
		};
		dx::XMMATRIX final_world_transform;
	};

	struct View_Desc
	{
		std::vector<Submesh_Render_Data> submeshes_info;
		std::vector<Diffusion_Profile_Handle> sss_profiles;
		std::vector<Light_Info> lights_info;
		uint8_t num_dir_lights;
		uint8_t num_point_lights;
		uint8_t num_spot_lights;
		Sky_Light skylight;
		Camera cam;
	};

	Node<Renderable_Entity>* build_renderable_entity_tree(Node<Renderable_Entity>* model_node, const Model3D* model, int current_mesh_index);

	class Scene_Manager
	{

	public:
		Scene_Manager();
		HRESULT init();

		//TODO: probably move somewhere else
		//Renderable_Entity load_model_from_file(const std::string& filename, bool optimize_graph = false, bool force_flatten_scene = false);
		void add_light(Node<Renderable_Entity>* attach_to, dx::XMVECTOR direction, float shadowmap_near_plane = 2.0f, float shadowmap_far_plane = 20.0f); //add directional light
		void add_light(Node<Renderable_Entity>* attach_to, dx::XMVECTOR direction, dx::XMVECTOR position, float cutoff_angle); //add spotlight
		void add_light(Node<Renderable_Entity>* attach_to, dx::XMVECTOR position, float constant, float linear, float quadratic); //add point light
		void add_skylight(Node<Renderable_Entity>* attach_to); //add skylight light

		u16 add_diffusion_profile(Diffusion_Profile_Handle hnd_profile);

		int remove_node(Node<Renderable_Entity>* node_to_remove, Node<Renderable_Entity>* parent_of_node);

		View_Desc compute_view(const Camera& cam);

		Material_Cache_Handle_t hnd_default_material;

		Scene_Graph<Renderable_Entity> m_scene_graph;

		//TODO: Maybe should be BVH instead of linear list? Remember Frostbite presentation.
		std::vector<Light_Info> m_scene_lights;

		std::vector<Vertex> m_static_scene_vertex_data; //mesh vertex data base indexed by part of submesh handle
		std::vector<uint16_t> m_static_scene_index_data; //mesh index data base indexed by part of submesh handle

	//private:
		//void load_node_children(const aiScene* scene, aiNode** children, unsigned int num_children, Renderable_Entity& parent_renderable_entity);
		// Renderable_Entity load_node_meshes(const aiScene* scene, unsigned int* meshes_indices, unsigned int num_meshes, Renderable_Entity& parent_renderable_entity, Transform_t local_tranform = IDENTITY_TRANSFORM);

		void parse_scene_graph(const Node<Renderable_Entity>* node, const dx::XMMATRIX& parent_transform, std::vector<Submesh_Render_Data>& submeshes_in_view, std::vector<Light_Info>& lights_in_view);

		std::vector<Diffusion_Profile_Handle> scene_dprofiles;

		Assimp::Importer m_importer;
		std::vector<int> m_freed_scene_light_indices;
		//std::vector<int> m_freed_scene_mesh_indices;
		//std::vector<int> m_freed_scene_material_indices;

		std::vector<Fragment_Info> m_freed_scene_static_vertex_data_fragments;
		std::vector<Fragment_Info> m_freed_scene_static_index_data_fragments;
	};


	extern Scene_Manager scene_manager;

}
#endif // !RENDERER_FRONTEND_H_
