#ifndef LIGHTS_H_
#define LIGHTS_H_

#include <DirectXMath.h>
#include <cstdint>

namespace dx = DirectX;

enum class LightType : std::uint8_t{
    DIRECTIONAL,
    POINT,
    SPOT
};

struct DirectionalLight {
    dx::XMVECTOR direction;
};

struct PointLight {
    dx::XMVECTOR posWorldSpace;

    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    dx::XMVECTOR posWorldSpace;
    dx::XMVECTOR direction;
    float cosCutoffAngle;
};

struct LightHandle_t {
    LightType tag;
    std::uint8_t index;
};

struct LightCB {
    DirectionalLight dLight;

    int numPLights;
    PointLight pointLights[16];
    dx::XMVECTOR posViewSpace[16];

    int numSpotLights;
    SpotLight spotLights[16];
    dx::XMVECTOR posSpotViewSpace[16];
    dx::XMVECTOR dirSpotViewSpace[16];
};

struct DirShadowCB {
    dx::XMMATRIX dirVMat;
    dx::XMMATRIX dirPMat;
};

struct OmniDirShadowCB {
    dx::XMMATRIX dirVMat[16*6];
    dx::XMMATRIX dirPMat;

    dx::XMMATRIX spotLightViewMat[16];
    dx::XMMATRIX spotLightProjMat[16];
};

#endif