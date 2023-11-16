#ifndef LIGHTS_H_
#define LIGHTS_H_

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
    int numPLights;
    PointLight pointLights[16];
};

extern DirectionalLight dLight;
extern PointLight pLight1;

#endif