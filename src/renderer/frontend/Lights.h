#ifndef LIGHTS_H_
#define LIGHTS_H_

#ifdef __cplusplus

#include <DirectXMath.h>
#include <cstdint>

#include "reflection/ReflectionAnnotation.h"

namespace zorya
{
    namespace dx = DirectX;

    using float4 = dx::XMFLOAT4;
    using float3 = dx::XMFLOAT3;
    using float2 = dx::XMFLOAT2;

#else

#endif

struct Directional_Light
{
    float4 dir;

    PROPERTY(asd)
        float near_plane_dist;
    PROPERTY(asd)
        float far_plane_dist;

    PROPERTY(asd)
        float indirect_light;
    float pad;
};

struct Point_Light
{
    float4 pos_world_space;

    PROPERTY(asd)
        float constant;
    PROPERTY(asd)
        float lin;
    PROPERTY(asd)
        float quadratic;

    PROPERTY(asd)
        float near_plane_dist;
    PROPERTY(asd)
        float far_plane_dist;

    float3 pad;
};

struct Spot_Light
{
    float4 pos_world_space;
    float4 direction;

    PROPERTY(asd)
        float cos_cutoff_angle;

    PROPERTY(asd)
        float near_plane_dist;
    PROPERTY(asd)
        float far_plane_dist;

    float pad;
};

#ifdef __cplusplus

enum class Light_Type : uint8_t
{
    DIRECTIONAL,
    POINT,
    SPOT
};

struct Light_Handle_t
{
    Light_Type tag;
    uint8_t index;
};

struct Scene_Lights
{
    Directional_Light dir_light;

    int num_point_lights;
    int num_spot_ligthts;
    int pad[2];

    Point_Light point_lights[16];
    dx::XMVECTOR point_pos_view_space[16];

    Spot_Light spot_lights[16];
    dx::XMVECTOR spot_pos_view_space[16];
    dx::XMVECTOR spot_dir_view_space[16];
};

struct Dir_Shadow_CB
{
    dx::XMMATRIX dir_light_shadow_view_mat;
    dx::XMMATRIX dir_light_shadow_proj_mat;
};

struct Omni_Dir_Shadow_CB
{
    dx::XMMATRIX dir_light_shadow_view_mat;
    dx::XMMATRIX dir_light_shadow_proj_mat;

    dx::XMMATRIX point_light_view_mat[16 * 6];
    dx::XMMATRIX point_light_proj_mat;

    dx::XMMATRIX spot_light_view_mat[16];
    dx::XMMATRIX spot_light_proj_mat[16];
};

}

#endif

#endif