#include "Lights.h"

DirectionalLight dLight = { dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f) };
PointLight pLight1 = { dx::XMVectorSet(2.0f, 2.0f, 0.0f, 1.0f), 1.0f, 0.22f, 0.20f };