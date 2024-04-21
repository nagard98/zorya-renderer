#ifndef MODEL_H_
#define MODEL_H_

#include <DirectXMath.h>
#include <vector>
#include <string>
#include <wrl/client.h>
#include <d3d11_1.h>

#include "Material.h"

//#include <BufferCache.h>

namespace zorya
{
	namespace wrl = Microsoft::WRL;
	namespace dx = DirectX;


	struct Model_Handle_t
	{
		uint16_t base_mesh;
		uint16_t num_meshes;
	};

	struct Submesh_Handle_t
	{
		uint16_t material_desc_id;
		uint32_t base_vertex;
		uint32_t base_index;
		uint32_t num_vertices;
		uint32_t num_indices;
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