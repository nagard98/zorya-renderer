#include "Shaders.h"
#include "RenderHardwareInterface.h"

#include <string>
#include <Windows.h>
#include <d3d11_1.h>

Shaders shaders;
D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[4] = {
    {"position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    { "texcoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "tangent", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

HRESULT Shaders::Init() {
    vertexShaders.resize((std::uint8_t)VShaderID::NUM_SHADERS);
    pixelShaders.resize((std::uint8_t)PShaderID::NUM_SHADERS);

    vertexLayout = nullptr;
    simpleVertexLayout = nullptr;

    BuildDefaultShaders();

    return S_OK;
}

HRESULT Shaders::BuildDefaultShaders() {
    ID3DBlob* shaderBlob = nullptr;
    HRESULT hRes;

    hRes = LoadShader<ID3D11VertexShader>(L"./shaders/StandardVertexShader.hlsl", "vs", &shaderBlob, &vertexShaders.at((std::uint8_t)VShaderID::STANDARD), rhi.device.Get());
    RETURN_IF_FAILED(hRes);
    hRes = rhi.device->CreateInputLayout(vertexLayoutDesc, 4, shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), &vertexLayout);
    RETURN_IF_FAILED(hRes);

    hRes = LoadShader<ID3D11VertexShader>(L"./shaders/DepthVertexShader.hlsl", "vs", &shaderBlob, &vertexShaders.at((std::uint8_t)VShaderID::DEPTH), rhi.device.Get());
    RETURN_IF_FAILED(hRes);
    hRes = LoadShader<ID3D11VertexShader>(L"./shaders/SkyboxVertexShader.hlsl", "vs", &shaderBlob, &vertexShaders.at((std::uint8_t)VShaderID::SKYBOX), rhi.device.Get());
    RETURN_IF_FAILED(hRes);
    hRes = LoadShader<ID3D11VertexShader>(L"./shaders/SSSSS.hlsl", "vs", &shaderBlob, &vertexShaders.at((std::uint8_t)VShaderID::POST_PROCESSING), rhi.device.Get());
    RETURN_IF_FAILED(hRes);


    hRes = LoadShader<ID3D11PixelShader>(L"./shaders/StandardPixelShader.hlsl", "ps", &shaderBlob, &pixelShaders.at((std::uint8_t)PShaderID::STANDARD), rhi.device.Get());
    RETURN_IF_FAILED(hRes);
    hRes = LoadShader<ID3D11PixelShader>(L"./shaders/SkyboxPixelShader.hlsl", "ps", &shaderBlob, &pixelShaders.at((std::uint8_t)PShaderID::SKYBOX), rhi.device.Get());
    RETURN_IF_FAILED(hRes);
    hRes = LoadShader<ID3D11PixelShader>(L"./shaders/SkinPixelShader.hlsl", "ps", &shaderBlob, &pixelShaders.at((std::uint8_t)PShaderID::SKIN), rhi.device.Get());
    RETURN_IF_FAILED(hRes);
    hRes = LoadShader<ID3D11PixelShader>(L"./shaders/SSSSS.hlsl", "ps", &shaderBlob, &pixelShaders.at((std::uint8_t)PShaderID::SSSSS), rhi.device.Get());
    RETURN_IF_FAILED(hRes);

    if (shaderBlob != nullptr) shaderBlob->Release();

    return S_OK;
}

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