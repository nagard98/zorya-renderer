#ifndef ZORYA_SHADER_H_
#define ZORYA_SHADER_H_

#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <cstdint>

enum class VShaderID : std::uint8_t {
	STANDARD,
	SKYBOX,
	DEPTH,
	FULL_QUAD,
	NUM_SHADERS
};

enum class PShaderID : std::uint8_t {
	UNDEFINED,
	STANDARD,
	SKYBOX,
	SKIN,
	SSSSS,
	LIGHTING,
	SHADOW_MAP,
	PRESENT,
	NUM_SHADERS
};


static D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[4] = {
	{"position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{ "texcoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "tangent", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};


struct ShaderBytecode {
	const BYTE* bytecode;
	size_t sizeInBytes;
};

struct ShaderTexture2D {
	ID3D11ShaderResourceView* resourceView;
};

struct PixelShader {
	ID3D11PixelShader* shader;
	ID3D11ShaderReflection* shaderReflection;

	static PixelShader create(const BYTE* shaderByteCode, size_t byteCodeSize);
	static PixelShader create(PShaderID pixelShaderId);

	void freeShader();

	HRESULT bindTexture2D(const char* bindingName, const ShaderTexture2D& texture);

	static ID3D11PixelShader* registeredPixelShaders[(std::uint8_t)PShaderID::NUM_SHADERS];
	static ShaderBytecode pixelShaderBytecodeBuffers[(std::uint8_t)PShaderID::NUM_SHADERS];
};


struct VertexShader {
	ID3D11VertexShader* shader;
	ID3D11InputLayout* vertexInputLayout;
	ID3D11ShaderReflection* shaderReflection;

	static VertexShader create(const BYTE* shaderByteCode, size_t byteCodeSize, D3D11_INPUT_ELEMENT_DESC* verteLayoutDesc, int numInputElements);
	static VertexShader create(VShaderID vertexShaderId, D3D11_INPUT_ELEMENT_DESC* verteLayoutDesc, int numInputElements);

	void freeShader();

	static ID3D11VertexShader* registeredVertexShaders[(std::uint8_t)VShaderID::NUM_SHADERS];
	static ShaderBytecode vertexShaderBytecodeBuffers[(std::uint8_t)VShaderID::NUM_SHADERS];
};

#endif