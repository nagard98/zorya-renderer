#ifndef SHADERS_H_
#define SHADERS_H_

#include <Windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <string>

template<class ShaderClass>
HRESULT CreateShader(ID3DBlob* pShaderBlob, ShaderClass** pShader, ID3D11Device* g_d3dDevice);

template<class ShaderClass>
std::string GetLatestProfile();


template<class ShaderClass>
HRESULT LoadShader(const std::wstring& shaderFilename, const std::string& entrypoint, ID3DBlob** shaderBlob, ShaderClass** shader, ID3D11Device* g_d3dDevice) {

    wrl::ComPtr<ID3DBlob> errBlob = nullptr;

    std::string profile = GetLatestProfile<ShaderClass>();

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG;

    HRESULT hr = D3DCompileFromFile(shaderFilename.c_str(), NULL, NULL, entrypoint.c_str(), profile.c_str(), flags, 0, shaderBlob, errBlob.GetAddressOf());

    if (FAILED(hr)) {
        std::string errorMessage = (char*)errBlob->GetBufferPointer();
        OutputDebugStringA(errorMessage.c_str());

        return hr;
    }

    hr = CreateShader<ShaderClass>(*shaderBlob, shader, g_d3dDevice);
    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}

#endif // ! SHADERS_H_