#ifndef MATERIAL_H_
#define MATERIAL_H_

#include <DirectXMath.h>
#include <string>
#include <cstdint>
#include "Shaders.h"

#include <d3d11.h>

namespace dx = DirectX;


constexpr std::uint8_t NO_UPDATE_MAT = 0;
constexpr std::uint8_t UPDATE_MAT_PRMS = 1;
constexpr std::uint8_t UPDATE_MAT_MAPS = 2;
constexpr std::uint8_t IS_FIRST_MAT_ALLOC = 4;


struct MaterialDesc {
	PShaderID shaderType;

	wchar_t albedoPath[128];
	dx::XMFLOAT4 baseColor;

	float smoothness;

	float metalness;
	wchar_t metalnessMask[128];

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
	dx::XMFLOAT4 baseColor;

	std::uint32_t hasAlbedoMap;
	std::uint32_t hasMetalnessMap;
	std::uint32_t hasNormalMap;

	float smoothness;
	float metalness;

	std::uint8_t pad[12];

};

struct Material {
	MaterialModel model;

	ShaderTexture2D albedoMap;
	ShaderTexture2D metalnessMap;
	ShaderTexture2D normalMap;

	MaterialParams matPrms;
};

#endif