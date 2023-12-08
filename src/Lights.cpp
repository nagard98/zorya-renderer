#include "Lights.h"

DirectionalLight dLight = { dx::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f) };
//PointLight pLights[2] = { PointLight{dx::XMVectorSet(-2.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0.22f, 0.20f}, PointLight{ dx::XMVectorSet(0.0f, 3.0f, -0.5f, 1.0f), 1.0f, 0.22f, 0.20f } };
PointLight pLights[1] = { PointLight{dx::XMVectorSet(0.0f, 2.0f, -3.0f, 1.0f), 1.0f, 0.22f, 0.20f}};
SpotLight spotLights[1] = { SpotLight{dx::XMVectorSet(0.0f, 0.0f, -4.0f, 1.0f), dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), 0.9f} };