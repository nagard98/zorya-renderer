#include <DirectXMath.h>

namespace dx = DirectX;

struct DirectionalLight {
    dx::XMVECTOR direction;
};

struct PointLight {
    dx::XMVECTOR pos;

    float constant;
    float linear;
    float quadratic;
};

struct LightCB {
    DirectionalLight dLight;
    PointLight pointLights[1];
};