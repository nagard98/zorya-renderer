#ifndef MATERIAL_H_
#define MATERIAL_H_

#include <DirectXMath.h>
#include <string>
#include <cstdint>
#include "Shaders.h"
#include <variant>

#include <d3d11.h>

namespace dx = DirectX;

typedef std::uint8_t MatUpdateFlags_;

constexpr MatUpdateFlags_ NO_UPDATE_MAT = 0;
constexpr MatUpdateFlags_ UPDATE_MAT_PRMS = 1;
constexpr MatUpdateFlags_ UPDATE_MAT_MAPS = 2;
constexpr MatUpdateFlags_ IS_FIRST_MAT_ALLOC = 4;

typedef std::uint8_t MatDescFlags_;

constexpr MatDescFlags_ SMOOTHNESS_IS_MAP = 1;
constexpr MatDescFlags_ METALNESS_IS_MAP = 2;

struct MaterialDesc {
	PShaderID shaderType;
	MatDescFlags_ unionTags;

	wchar_t albedoPath[128];
	dx::XMFLOAT4 baseColor;
	dx::XMFLOAT4 subsurfaceAlbedo;
	dx::XMFLOAT4 meanFreePathColor;
	float meanFreePathDistance;
	float scale;

	union {
		float smoothnessValue;
		wchar_t smoothnessMap[128];
	};

	union {
		float metalnessValue;
		wchar_t metalnessMap[128];
	};

	wchar_t normalPath[128];
};

struct Texture2D {
	ID3D11Texture2D* resource;
};

struct ShaderTexture2D {
	ID3D11ShaderResourceView* resourceView;
};

struct MaterialModel {
	ID3D11PixelShader* shader;
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
	//std::uint8_t pad[4];
};


struct Material {
	MaterialModel model;

	ShaderTexture2D albedoMap;
	ShaderTexture2D metalnessMap;
	ShaderTexture2D normalMap;
	ShaderTexture2D smoothnessMap;

	MaterialParams matPrms;
};

#endif