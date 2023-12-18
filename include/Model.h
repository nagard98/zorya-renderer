#ifndef MODEL_H_
#define MODEL_H_

#include <DirectXMath.h>
#include <vector>
#include <string>
#include <wrl/client.h>
#include <d3d11_1.h>

#include "Material.h"

//#include <BufferCache.h>

namespace wrl = Microsoft::WRL;
namespace dx = DirectX;


struct ModelHandle_t {
    std::uint16_t baseMesh;
    std::uint16_t numMeshes;
};

struct SubmeshHandle_t {
    std::uint16_t matDescIdx;
    std::uint32_t baseVertex;
    std::uint32_t baseIndex;
    std::uint32_t numVertices;
    std::uint32_t numIndexes;
};

struct SimpleVertex
{
    SimpleVertex(float x, float y, float z) : position(x, y, z) {}

    dx::XMFLOAT3 position;
};

struct Vertex
{
    Vertex(float x, float y, float z,
        float u, float v, float nx, float ny, float nz, float tx, float ty, float tz)
        : position(x, y, z), texCoord(u, v), normal(nx, ny, nz), tangent(tx, ty, tz) {}

    dx::XMFLOAT3 position;
    dx::XMFLOAT2 texCoord;
    dx::XMFLOAT3 normal;
    dx::XMFLOAT3 tangent;
};

extern std::vector<Vertex> cubeVertices;
extern std::vector<std::uint16_t> cubeIndices;

extern std::vector<SimpleVertex> quadVertices;

#endif // !MODEL_H_