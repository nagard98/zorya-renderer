#include <DirectXMath.h>
#include <vector>

namespace dx = DirectX;

struct Vertex
{
    Vertex() {}
    Vertex(float x, float y, float z,
        float u, float v, float nx, float ny, float nz)
        : position(x, y, z), texCoord(u, v), normal(nx, ny, nz) {}

    dx::XMFLOAT3 position;
    dx::XMFLOAT2 texCoord;
    dx::XMFLOAT3 normal;
};

struct RenderSystem {
    std::vector<Vertex> vertices;
    std::vector<std::uint16_t> bufLengths;
    int k;
};