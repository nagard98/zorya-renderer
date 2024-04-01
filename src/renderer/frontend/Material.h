#ifndef MATERIAL_H_
#define MATERIAL_H_

#include <DirectXMath.h>
#include <string>
#include <cstdint>
#include <d3d11.h>

#include "Shader.h"
#include "reflection/ReflectionAnnotation.h"

namespace dx = DirectX;

enum class SSS_MODEL : std::uint32_t {
	NONE,
	JIMENEZ_GAUSS,
	JIMENEZ_SEPARABLE,
	GOLUBEV
};

struct JimenezSSSModel{
	PROPERTY()
	dx::XMFLOAT4 subsurfaceAlbedo;

	PROPERTY()
	dx::XMFLOAT4 meanFreePathColor;

	PROPERTY()
	float meanFreePathDistance;

	PROPERTY()
	float scale;

	PROPERTY()
	std::uint32_t numSamples;
};

struct MultiOption {
	std::uint8_t selectedOption;

	union {
		JimenezSSSModel sssModel;
	};
};


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
	dx::XMFLOAT4 baseColor;

	PROPERTY()
	wchar_t albedoPath[128];

	union {
		float smoothnessValue;
		PROPERTY()
		wchar_t smoothnessMap[128];
	};

	union {
		PROPERTY()
		float metalnessValue;
		wchar_t metalnessMap[128];
	};

	PROPERTY()
	wchar_t normalPath[128];

	SSS_MODEL selectedSSSModel;

	PROPERTY()
	JimenezSSSModel sssModel;


};


struct Texture2D {
	ID3D11Texture2D* resource;
};

struct SubsurfaceScatteringParams {
	float samples[64];
	dx::XMFLOAT4 subsurfaceAlbedo;
	dx::XMFLOAT4 meanFreePathColor;
	float meanFreePathDist;
	float scale;
	dx::XMFLOAT2 dir;
	std::uint32_t numSamples;
	std::uint32_t pad[3];

	dx::XMFLOAT4 jimenezSamplesSSS[15];
	//dx::XMFLOAT4 kernel[16];
};

struct MaterialParams {
	dx::XMFLOAT4 baseColor;

	std::uint32_t hasAlbedoMap;
	std::uint32_t hasMetalnessMap;
	std::uint32_t hasNormalMap;
	std::uint32_t hasSmoothnessMap;

	float roughness;
	float metalness;

	std::uint32_t sssModelId;
	std::uint8_t pad[4];
};


struct Material {
	PixelShader model;

	ShaderTexture2D albedoMap;
	ShaderTexture2D metalnessMap;
	ShaderTexture2D normalMap;
	ShaderTexture2D smoothnessMap;

	SubsurfaceScatteringParams sssPrms;
	MaterialParams matPrms;
};

#endif