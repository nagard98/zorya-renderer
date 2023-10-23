#include "../include/Shaders.h"

#include <string>
#include <Windows.h>
#include <d3d11_1.h>

template<>
HRESULT CreateShader<ID3D11VertexShader>(ID3DBlob* pShaderBlob, ID3D11VertexShader** pShader, ID3D11Device* g_d3dDevice)
{
    return g_d3dDevice->CreateVertexShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, pShader);
};

template<>
HRESULT CreateShader<ID3D11PixelShader>(ID3DBlob* pShaderBlob, ID3D11PixelShader** pShader, ID3D11Device* g_d3dDevice)
{
    return g_d3dDevice->CreatePixelShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), nullptr, pShader);
};

template<>
std::string GetLatestProfile<ID3D11VertexShader>() {
    return "vs_5_0";
}

template<>
std::string GetLatestProfile<ID3D11PixelShader>() {
    return "ps_5_0";
}