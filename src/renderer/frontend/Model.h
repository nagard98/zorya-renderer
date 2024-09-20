 #ifndef MODEL_H_
#define MODEL_H_

#include <DirectXMath.h>
#include <vector>
#include <string>
#include <wrl/client.h>
#include <d3d11_1.h>

#include "Material.h"
#include "Asset.h"
#include "Transform.h"
#include "renderer/backend/PipelineStateObject.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

//#include <BufferCache.h>

namespace zorya
{
	namespace wrl = Microsoft::WRL;
	namespace dx = DirectX;

	struct Submesh_Handle_t
	{
		uint16_t material_desc_id;
		uint32_t base_vertex;
		uint32_t base_index;
		uint32_t num_vertices;
		uint32_t num_indices;
	};

	struct Submesh_Collection
	{
		//TODO: can probably be smaller
		std::vector<int> submesh_indices;
		std::vector<int> collection_indices;
		uint32_t depth;
		Transform_t local_transform;
	};


	class Model3D: public Asset
	{
	public:
		static Model3D* create(const Asset_Import_Config* asset_imp_conf);

		Model3D(const Asset_Import_Config* asset_imp_conf);

		void load_asset(const Asset_Import_Config* tex_imp_config) override {};

		Material_Cache_Handle_t global_material;
		std::vector<Material*> submesh_materials;
		//TODO: dont use pso_desc, but something higher level
		//std::vector<PSO_Desc> submesh_psos;
		std::vector<Submesh_Handle_t> submeshes;
		std::vector<std::string> submesh_names;
		std::vector<Submesh_Collection> submesh_collections;
		std::vector<std::string> collection_names;

	private:
		std::vector<int> load_node_children(const aiScene* scene, aiNode** children, unsigned int num_children, unsigned int depth);
		std::vector<int> load_node_meshes(const aiScene* scene, unsigned int* meshes_indices, unsigned int num_meshes, Transform_t local_transform = IDENTITY_TRANSFORM);

	};


	struct Simple_Vertex
	{
		Simple_Vertex(float x, float y, float z) : position(x, y, z) {}

		dx::XMFLOAT3 position;
	};

	struct Vertex
	{
		Vertex(float x, float y, float z,
			float u, float v, float nx, float ny, float nz, float tx, float ty, float tz)
			: position(x, y, z), tex_coord(u, v), normal(nx, ny, nz), tangent(tx, ty, tz)
		{}

		dx::XMFLOAT3 position;
		dx::XMFLOAT2 tex_coord;
		dx::XMFLOAT3 normal;
		dx::XMFLOAT3 tangent;
	};

	extern std::vector<Vertex> cube_vertices;
	extern std::vector<uint16_t> cube_indices;

	extern std::vector<Simple_Vertex> quad_vertices;

}

#endif // !MODEL_H_