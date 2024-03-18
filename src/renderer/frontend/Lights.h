#ifndef LIGHTS_H_
#define LIGHTS_H_

#include <DirectXMath.h>
#include <cstdint>

#include "reflection/Reflection.h"

namespace dx = DirectX;

enum class LightType : std::uint8_t{
    DIRECTIONAL,
    POINT,
    SPOT
};

struct DirectionalLight {
    dx::XMVECTOR direction;

    PROPERTY()
    float shadowMapNearPlane;
    
    PROPERTY()
    float shadowMapFarPlane;

    float pad[2];
};

struct PointLight {
    dx::XMVECTOR posWorldSpace;

    PROPERTY()
    float constant;
    
    PROPERTY()
    float linear;
    
    PROPERTY()
    float quadratic;


    PROPERTY()
    float shadowMapNearPlane;
    
    PROPERTY()
    float shadowMapFarPlane;
    
    float pad[3];
};

struct SpotLight {
    dx::XMVECTOR posWorldSpace;
    dx::XMVECTOR direction;

    PROPERTY()
    float cosCutoffAngle;


    PROPERTY()
    float shadowMapNearPlane;
    
    PROPERTY()
    float shadowMapFarPlane;

    float pad;
};

struct LightHandle_t {
    LightType tag;
    std::uint8_t index;
};

struct LightCB {
    DirectionalLight dLight;

    int numPLights;
    int numSpotLights;
    int pad[2];

    PointLight pointLights[16];
    dx::XMVECTOR posViewSpace[16];

    SpotLight spotLights[16];
    dx::XMVECTOR posSpotViewSpace[16];
    dx::XMVECTOR dirSpotViewSpace[16];
};

struct DirShadowCB {
    dx::XMMATRIX dirVMat;
    dx::XMMATRIX dirPMat;
};

struct OmniDirShadowCB {
    dx::XMMATRIX dirLightViewMat;
    dx::XMMATRIX dirLightProjMat;

    dx::XMMATRIX pointLightViewMat[16*6];
    dx::XMMATRIX pointLightProjMat;

    dx::XMMATRIX spotLightViewMat[16];
    dx::XMMATRIX spotLightProjMat[16];
};

#endif