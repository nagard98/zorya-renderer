#include <DirectXMath.h>
#include <vector>
#include <string>
#include <wrl/client.h>
#include <d3d11_1.h>

#include <BufferCache.h>

namespace wrl = Microsoft::WRL;
namespace dx = DirectX;

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

struct SubmeshTriangles {
    std::vector<Vertex> vertices;
    std::vector<WORD> indices;

    std::uint64_t vertexBuffer;
    std::uint64_t indexBuffer;
};

struct Submesh {
    SubmeshTriangles submeshTriangles;
    //Material material;
};

struct Model {
    std::vector<Submesh> submeshes;
    //Bound aabb;
};


struct Meshes {
    Meshes() : vertexBuffers(10), indexBuffers(10) {}

    std::vector<wrl::ComPtr<ID3D11Buffer>> vertexBuffers;
    std::vector<wrl::ComPtr<ID3D11Buffer>> indexBuffers;
};

struct RenderSystem {
    std::vector<Vertex> vertices;
    std::vector<std::uint16_t> bufLengths;
    int k;
};