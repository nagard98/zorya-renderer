#ifndef LIGHTS_H_
#define LIGHTS_H_

#ifdef __cplusplus

#include <DirectXMath.h>
#include <cstdint>

#include "reflection/Reflection.h"

namespace dx = DirectX;

using float4 = dx::XMFLOAT4;
using float3 = dx::XMFLOAT3;
using float2 = dx::XMFLOAT2;

#else

#endif

struct DirectionalLight
{
    float4 dir;

    PROPERTY(asd)
    float nearPlaneDist;
    PROPERTY(asd)
    float farPlaneDist;

    PROPERTY(asd)
    float ambient;
    float pad;
};

struct PointLight
{
    float4 posWorldSpace;

    PROPERTY(asd)
    float constant;
    PROPERTY(asd)
    float lin;
    PROPERTY(asd)
    float quad;

    PROPERTY(asd)
    float nearPlaneDist;
    PROPERTY(asd)
    float farPlaneDist;

    float3 pad;
};

struct SpotLight
{
    float4 posWorldSpace;
    float4 direction;

    PROPERTY(asd)
    float cosCutoffAngle;

    PROPERTY(asd)
    float nearPlaneDist;
    PROPERTY(asd)
    float farPlaneDist;

    float pad;
};

#ifdef __cplusplus

enum class LightType : std::uint8_t {
    DIRECTIONAL,
    POINT,
    SPOT
};

struct LightHandle_t {
    LightType tag;
    std::uint8_t index;
};

struct SceneLights {
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

#endif