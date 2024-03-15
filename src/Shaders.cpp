#include "Shaders.h"
#include "RenderHardwareInterface.h"

#include <string>
#include <Windows.h>
#include <d3d11_1.h>

#include "shaders/DepthPS.h"
#include "shaders/DepthVS.h"
#include "shaders/GBufferPS.h"
#include "shaders/GBufferVS.h"
#include "shaders/Lighting.h"
#include "shaders/Present.h"
#include "shaders/ShadowMapping.h"
#include "shaders/SkyboxPS.h"
#include "shaders/SkyboxVS.h"
#include "shaders/SSSSS.h"
#include "shaders/FullscreenQuad.h"

Shaders shaders;

D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[4] = {
    {"position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    { "texcoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "tangent", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

Shaders::~Shaders()
{
    for (ID3D11PixelShader* ps : pixelShaders) {
        if (ps) ps->Release();
    }
    
    for (ID3D11VertexShader* vs : vertexShaders) {
        if (vs) vs->Release();
    }
}

HRESULT Shaders::Init() {
    vertexShaders.resize((std::uint8_t)VShaderID::NUM_SHADERS);
    pixelShaders.resize((std::uint8_t)PShaderID::NUM_SHADERS);

    vertexLayout = nullptr;
    simpleVertexLayout = nullptr;

    HRESULT hr = LoadDefaultShaders();
    //BuildDefaultShaders();

    return hr;
}

HRESULT Shaders::BuildDefaultShaders() {
    ID3DBlob* shaderBlob = nullptr;
    HRESULT hRes;

    hRes = LoadShader<ID3D11VertexShader>(L"./shaders/GBufferVS.hlsl", "vs", &shaderBlob, &vertexShaders.at((std::uint8_t)VShaderID::STANDARD), rhi.device.device.Get());
    RETURN_IF_FAILED(hRes);
    hRes = rhi.device.device->CreateInputLayout(vertexLayoutDesc, 4, shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), &vertexLayout);
    RETURN_IF_FAILED(hRes);

    hRes = LoadShader<ID3D11VertexShader>(L"./shaders/DepthVS.hlsl", "vs", &shaderBlob, &vertexShaders.at((std::uint8_t)VShaderID::DEPTH), rhi.device.device.Get());
    RETURN_IF_FAILED(hRes);
    hRes = LoadShader<ID3D11VertexShader>(L"./shaders/SkyboxVS.hlsl", "vs", &shaderBlob, &vertexShaders.at((std::uint8_t)VShaderID::SKYBOX), rhi.device.device.Get());
    RETURN_IF_FAILED(hRes);
    hRes = LoadShader<ID3D11VertexShader>(L"./shaders/FullcreenQuad.hlsl", "vs", &shaderBlob, &vertexShaders.at((std::uint8_t)VShaderID::FULL_QUAD), rhi.device.device.Get());
    RETURN_IF_FAILED(hRes);

    
    hRes = LoadShader<ID3D11PixelShader>(L"./shaders/GBufferPS.hlsl", "ps", &shaderBlob, &pixelShaders.at((std::uint8_t)PShaderID::STANDARD), rhi.device.device.Get());
    RETURN_IF_FAILED(hRes);
    hRes = LoadShader<ID3D11PixelShader>(L"./shaders/SkyboxPS.hlsl", "ps", &shaderBlob, &pixelShaders.at((std::uint8_t)PShaderID::SKYBOX), rhi.device.device.Get());
    RETURN_IF_FAILED(hRes);
    hRes = LoadShader<ID3D11PixelShader>(L"./shaders/SSSSS.hlsl", "ps", &shaderBlob, &pixelShaders.at((std::uint8_t)PShaderID::SSSSS), rhi.device.device.Get());
    RETURN_IF_FAILED(hRes);
    hRes = LoadShader<ID3D11PixelShader>(L"./shaders/Lighting.hlsl", "ps", &shaderBlob, &pixelShaders.at((std::uint8_t)PShaderID::LIGHTING), rhi.device.device.Get());
    RETURN_IF_FAILED(hRes);
    hRes = LoadShader<ID3D11PixelShader>(L"./shaders/ShadowMapping.hlsl", "ps", &shaderBlob, &pixelShaders.at((std::uint8_t)PShaderID::SHADOW_MAP), rhi.device.device.Get());
    RETURN_IF_FAILED(hRes);
    hRes = LoadShader<ID3D11PixelShader>(L"./shaders/Present.hlsl", "ps", &shaderBlob, &pixelShaders.at((std::uint8_t)PShaderID::PRESENT), rhi.device.device.Get());
    RETURN_IF_FAILED(hRes);

    if (shaderBlob != nullptr) shaderBlob->Release();

    return hRes;
}

HRESULT Shaders::LoadDefaultShaders()
{
    HRESULT hRes;

    hRes = rhi.device.device->CreateVertexShader(g_GBufferVS, sizeof(g_GBufferVS), nullptr, &vertexShaders.at((std::uint8_t)VShaderID::STANDARD));
    RETURN_IF_FAILED(hRes);
    hRes = rhi.device.device->CreateInputLayout(vertexLayoutDesc, 4, g_GBufferVS, sizeof(g_GBufferVS), &vertexLayout);
    RETURN_IF_FAILED(hRes);

    hRes = rhi.device.device->CreateVertexShader(g_DepthVS, sizeof(g_DepthVS), nullptr, &vertexShaders.at((std::uint8_t)VShaderID::DEPTH));
    RETURN_IF_FAILED(hRes);
    hRes = rhi.device.device->CreateVertexShader(g_SkyboxVS, sizeof(g_SkyboxVS), nullptr, &vertexShaders.at((std::uint8_t)VShaderID::SKYBOX));
    RETURN_IF_FAILED(hRes);
    hRes = rhi.device.device->CreateVertexShader(g_FullscreenQuad, sizeof(g_FullscreenQuad), nullptr, &vertexShaders.at((std::uint8_t)VShaderID::FULL_QUAD));
    RETURN_IF_FAILED(hRes);

    hRes = rhi.device.device->CreatePixelShader(g_GBufferPS, sizeof(g_GBufferPS), nullptr, &pixelShaders.at((std::uint8_t)PShaderID::STANDARD));
    RETURN_IF_FAILED(hRes);
    hRes = rhi.device.device->CreatePixelShader(g_SkyboxPS, sizeof(g_SkyboxPS), nullptr, &pixelShaders.at((std::uint8_t)PShaderID::SKYBOX));
    RETURN_IF_FAILED(hRes);
    hRes = rhi.device.device->CreatePixelShader(g_SSSSS, sizeof(g_SSSSS), nullptr, &pixelShaders.at((std::uint8_t)PShaderID::SSSSS));
    RETURN_IF_FAILED(hRes);
    hRes = rhi.device.device->CreatePixelShader(g_Lighting, sizeof(g_Lighting), nullptr, &pixelShaders.at((std::uint8_t)PShaderID::LIGHTING));
    RETURN_IF_FAILED(hRes);
    hRes = rhi.device.device->CreatePixelShader(g_ShadowMapping, sizeof(g_ShadowMapping), nullptr, &pixelShaders.at((std::uint8_t)PShaderID::SHADOW_MAP));
    RETURN_IF_FAILED(hRes);
    hRes = rhi.device.device->CreatePixelShader(g_Present, sizeof(g_Present), nullptr, &pixelShaders.at((std::uint8_t)PShaderID::PRESENT));
    RETURN_IF_FAILED(hRes);

    return hRes;
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