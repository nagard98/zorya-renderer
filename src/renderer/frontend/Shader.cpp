#include "Shader.h"
#include "renderer/backend/rhi/RenderHardwareInterface.h"

//#include "shaders/headers/DepthPS.h"
//#include "shaders/headers/DepthVS.h"
//#include "shaders/headers/GBufferPS.h"
//#include "shaders/headers/GBufferVS.h"
//#include "shaders/headers/Lighting.h"
//#include "shaders/headers/Present.h"
//#include "shaders/headers/ShadowMapping.h"
//#include "shaders/headers/SkyboxPS.h"
//#include "shaders/headers/SkyboxVS.h"
//#include "shaders/headers/SSSSS.h"
//#include "shaders/headers/FullscreenQuad.h"

#include <cstdio>

//TODO: implement also shader unload from memory
constexpr ShaderBytecode loadShaderBytecode(const char* filepath) {
	FILE* file = nullptr;
	BYTE* bytecode = nullptr;
	size_t sizeInBytes = 0;

	if (fopen_s(&file, filepath, "rb") == 0) {
		fseek(file, 0, SEEK_END);
		sizeInBytes = ftell(file);
		fseek(file, 0, SEEK_SET);
		
		bytecode = (BYTE*)malloc(sizeInBytes);

		fread_s(bytecode, sizeInBytes, sizeof(BYTE), sizeInBytes/sizeof(BYTE), file);
		fclose(file);
	}
	
	return ShaderBytecode{ bytecode, sizeInBytes };
}

ID3D11PixelShader* PixelShader::registeredPixelShaders[] = { nullptr };
ShaderBytecode PixelShader::pixelShaderBytecodeBuffers[] = {
	ShaderBytecode{},
	loadShaderBytecode("./shaders/GBufferPS.cso"), //ShaderBytecode{ g_GBufferPS, sizeof(g_GBufferPS) },
	loadShaderBytecode("./shaders/SkyboxPS.cso"), //ShaderBytecode{ g_SkyboxPS, sizeof(g_SkyboxPS) },
	ShaderBytecode{},
	loadShaderBytecode("./shaders/SSSSS.cso"), //ShaderBytecode{ g_SSSSS, sizeof(g_SSSSS) },
	loadShaderBytecode("./shaders/Lighting.cso"), //ShaderBytecode{ g_Lighting, sizeof(g_Lighting) },
	loadShaderBytecode("./shaders/ShadowMapping.cso"), //ShaderBytecode{ g_ShadowMapping, sizeof(g_ShadowMapping) },
	loadShaderBytecode("./shaders/Present.cso"), //ShaderBytecode{ g_Present, sizeof(g_Present) }
};

PixelShader PixelShader::create(const BYTE* shaderByteCode, size_t byteCodeSize){
	ID3D11PixelShader* _shader = nullptr;
	HRESULT hRes = rhi.device.device->CreatePixelShader(shaderByteCode, byteCodeSize, nullptr, &_shader);

	ID3D11ShaderReflection* _shaderReflection = nullptr;
	hRes = D3DReflect(shaderByteCode, byteCodeSize, IID_ID3D11ShaderReflection, (void**)&_shaderReflection);

	return PixelShader{ _shader, _shaderReflection };
}

PixelShader PixelShader::create(PShaderID pixelShaderId){
	HRESULT hRes = S_OK;

	ShaderBytecode shaderBytecode = pixelShaderBytecodeBuffers[(std::uint8_t)pixelShaderId];
	ID3D11PixelShader* _shader = registeredPixelShaders[(std::uint8_t)pixelShaderId];

	if (_shader == nullptr) {
		hRes = rhi.device.device->CreatePixelShader(shaderBytecode.bytecode, shaderBytecode.sizeInBytes, nullptr, &_shader);
		if (!FAILED(hRes)) registeredPixelShaders[(std::uint8_t)pixelShaderId] = _shader;
	}
	else {
		_shader->AddRef();
	}

	ID3D11ShaderReflection* _shaderReflection = nullptr;
	hRes = D3DReflect(shaderBytecode.bytecode, shaderBytecode.sizeInBytes, IID_ID3D11ShaderReflection, (void**)&_shaderReflection);

	return PixelShader{ _shader, _shaderReflection };
}

void PixelShader::freeShader()
{
	if (shader)shader->Release();
	if (shaderReflection) shaderReflection->Release();
	//TODO: IMPORTANT! Add removal from registered shaders (in shader struct add its id, to know where to remove)
}

HRESULT PixelShader::bindTexture2D(const char* bindingName, const ShaderTexture2D& texture)
{
	D3D11_SHADER_INPUT_BIND_DESC bindDesc;
	HRESULT hr = shaderReflection->GetResourceBindingDescByName(bindingName, &bindDesc);
	RETURN_IF_FAILED(hr);
	
	rhi.context->PSSetShaderResources(bindDesc.BindPoint, 1, &texture.resourceView);

	return S_OK;
}

HRESULT PixelShader::bindConstantBuffer(const char* bindingName, ID3D11Buffer* const constantBuffer)
{
	D3D11_SHADER_INPUT_BIND_DESC bindDesc;
	HRESULT hr  = shaderReflection->GetResourceBindingDescByName(bindingName, &bindDesc);
	RETURN_IF_FAILED(hr);

	rhi.context->PSSetConstantBuffers(bindDesc.BindPoint, 1, &constantBuffer);

	return hr;
}





VertexShader VertexShader::create(const BYTE* shaderByteCode, size_t byteCodeSize, D3D11_INPUT_ELEMENT_DESC* verteLayoutDesc, int numInputElements) {
	ID3D11VertexShader* _shader = nullptr;
	HRESULT hRes = rhi.device.device->CreateVertexShader(shaderByteCode, byteCodeSize, nullptr, &_shader);

	ID3D11InputLayout* _vertexInputLayout = nullptr;
	if (verteLayoutDesc != nullptr) {
		hRes = rhi.device.device->CreateInputLayout(verteLayoutDesc, numInputElements, shaderByteCode, byteCodeSize, &_vertexInputLayout);
	}

	ID3D11ShaderReflection* _shaderReflection = nullptr;
	hRes = D3DReflect(shaderByteCode, byteCodeSize, IID_ID3D11ShaderReflection, (void**)&_shaderReflection);

	return VertexShader{ _shader, _vertexInputLayout, _shaderReflection };
}

VertexShader VertexShader::create(VShaderID vertexShaderId, D3D11_INPUT_ELEMENT_DESC* verteLayoutDesc, int numInputElements) {
	HRESULT hRes = S_OK;

	ShaderBytecode shaderBytecode = vertexShaderBytecodeBuffers[(std::uint8_t)vertexShaderId];
	ID3D11VertexShader* _shader = registeredVertexShaders[(std::uint8_t)vertexShaderId];

	if (_shader == nullptr) {
		hRes = rhi.device.device->CreateVertexShader(shaderBytecode.bytecode, shaderBytecode.sizeInBytes, nullptr, &_shader);
		if (!FAILED(hRes)) registeredVertexShaders[(std::uint8_t)vertexShaderId] = _shader;
	}
	else {
		_shader->AddRef();
	}

	ID3D11InputLayout* _vertexInputLayout = nullptr;
	if (verteLayoutDesc != nullptr) {
		hRes = rhi.device.device->CreateInputLayout(verteLayoutDesc, numInputElements, shaderBytecode.bytecode, shaderBytecode.sizeInBytes, &_vertexInputLayout);
	}

	ID3D11ShaderReflection* _shaderReflection = nullptr;
	hRes = D3DReflect(shaderBytecode.bytecode, shaderBytecode.sizeInBytes, IID_ID3D11ShaderReflection, (void**)&_shaderReflection);

	return VertexShader{ _shader, _vertexInputLayout, _shaderReflection };
}

void VertexShader::freeShader()
{
	if (shader)shader->Release();
	if (shaderReflection) shaderReflection->Release();
	if (vertexInputLayout)vertexInputLayout->Release();
	//TODO: IMPORTANT! Add removal from registered shaders (in shader struct add its id, to know where to remove)
}

HRESULT VertexShader::bindConstantBuffer(const char* bindingName, ID3D11Buffer* const constantBuffer)
{
	D3D11_SHADER_INPUT_BIND_DESC bindDesc;
	HRESULT hr = shaderReflection->GetResourceBindingDescByName(bindingName, &bindDesc);
	RETURN_IF_FAILED(hr);

	rhi.context->PSSetConstantBuffers(bindDesc.BindPoint, 1, &constantBuffer);

	return hr;
}

ID3D11VertexShader* VertexShader::registeredVertexShaders[] = { nullptr };
ShaderBytecode VertexShader::vertexShaderBytecodeBuffers[] = {
	loadShaderBytecode("./shaders/GBufferVS.cso"), //ShaderBytecode{ g_GBufferVS, sizeof(g_GBufferVS) },
	loadShaderBytecode("./shaders/SkyboxVS.cso"), //ShaderBytecode{ g_SkyboxVS, sizeof(g_SkyboxVS) },
	loadShaderBytecode("./shaders/DepthVS.cso"), //ShaderBytecode{ g_DepthVS, sizeof(g_DepthVS) },
	loadShaderBytecode("./shaders/FullscreenQuad.cso"), //ShaderBytecode{ g_FullscreenQuad, sizeof(g_FullscreenQuad) }
};