#ifndef MATERIAL_H_
#define MATERIAL_H_

#include <DirectXMath.h>
#include <string>

#include <d3d11.h>

namespace dx = DirectX;

enum class SHADER_TYPE {
	STANDARD
};

struct MaterialDesc {
	SHADER_TYPE shaderType;

	const wchar_t* albedoPath;
	dx::XMFLOAT4 baseColor;

	float smoothness;

	float metalness;
	const wchar_t* metalnessMask;

	const wchar_t* normalPath;
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