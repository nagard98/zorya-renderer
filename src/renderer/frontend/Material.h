#ifndef MATERIAL_H_
#define MATERIAL_H_

#include <DirectXMath.h>
#include <string>
#include <cstdint>
#include <d3d11.h>

#include "Shader.h"
#include "reflection/Reflection.h"

namespace dx = DirectX;

using MaterialDesc = ReflectionBase;

typedef std::uint8_t MatUpdateFlags_;

constexpr MatUpdateFlags_ NO_UPDATE_MAT = 0;
constexpr MatUpdateFlags_ UPDATE_MAT_PRMS = 1;
constexpr MatUpdateFlags_ UPDATE_MAT_MAPS = 2;
constexpr MatUpdateFlags_ IS_FIRST_MAT_ALLOC = 4;

typedef std::uint8_t MatDescFlags_;

constexpr MatDescFlags_ SMOOTHNESS_IS_MAP = 1;
constexpr MatDescFlags_ METALNESS_IS_MAP = 2;

struct StandardMaterialDesc {
	PShaderID shaderType;
	MatDescFlags_ unionTags;

	PROPERTY()
	wchar_t albedoPath[128];

	PROPERTY()
	dx::XMFLOAT4 baseColor;

	PROPERTY()
	dx::XMFLOAT4 subsurfaceAlbedo;

	PROPERTY()
	dx::XMFLOAT4 meanFreePathColor;

	PROPERTY()
	float meanFreePathDistance;

	PROPERTY()
	float scale;

	union {
		PROPERTY()
		float smoothnessValue;
		wchar_t smoothnessMap[128];
	};

	union {
		PROPERTY()
		float metalnessValue;
		wchar_t metalnessMap[128];
	};

	PROPERTY()
	wchar_t normalPath[128];
};


struct Texture2D {
	ID3D11Texture2D* resource;
};


struct MaterialParams {
	float samples[64];
	dx::XMFLOAT4 baseColor;
	dx::XMFLOAT4 subsurfaceAlbedo;

	dx::XMFLOAT4 meanFreePathColor;

	std::uint32_t hasAlbedoMap;
	std::uint32_t hasMetalnessMap;
	std::uint32_t hasNormalMap;
	std::uint32_t hasSmoothnessMap;

	float roughness;

	float metalness;
	float meanFreePathDist;
	float scale;

	
	dx::XMFLOAT4 kernel[16];
	dx::XMFLOAT2 dir;
	std::uint8_t pad[8];
	//std::uint8_t pad[4];
};


struct Material {
	PixelShader model;

	ShaderTexture2D albedoMap;
	ShaderTexture2D metalnessMap;
	ShaderTexture2D normalMap;
	ShaderTexture2D smoothnessMap;

	MaterialParams matPrms;
};

#endif