#ifndef SHADERS_H_
#define SHADERS_H_

#include <Windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <string>
#include <cstdint>
#include <vector>

#include <Editor/Logger.h>

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

class Shaders {
public:

    HRESULT Init();
    HRESULT BuildDefaultShaders();

    std::vector<ID3D11VertexShader*> vertexShaders;
    std::vector<ID3D11PixelShader*> pixelShaders;

    ID3D11InputLayout* vertexLayout;
    ID3D11InputLayout* simpleVertexLayout;
};

extern Shaders shaders;

extern std::vector<ID3D11VertexShader*> vertexShaders;
extern std::vector<ID3D11PixelShader*> pixelShaders;

extern D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[4];

template<class ShaderClass>
HRESULT CreateShader(ID3DBlob* pShaderBlob, ShaderClass** pShader, ID3D11Device* g_d3dDevice);

template<class ShaderClass>
std::string GetLatestProfile();


template<class ShaderClass>
HRESULT LoadShader(const std::wstring& shaderFilename, const std::string& entrypoint, ID3DBlob** shaderBlob, ShaderClass** shader, ID3D11Device* g_d3dDevice) {

    wrl::ComPtr<ID3DBlob> errBlob = nullptr;

    std::string profile = GetLatestProfile<ShaderClass>();

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG;

    HRESULT hr = D3DCompileFromFile(shaderFilename.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, entrypoint.c_str(), profile.c_str(), flags, 0, shaderBlob, errBlob.GetAddressOf());

    if (FAILED(hr)) {
        char* errMsg = (char*)errBlob->GetBufferPointer();

        for (int i = 0, first = 0; errMsg[i] != '\0'; i++) {
            if (errMsg[i] == '\n') {
                int numChar = i - first + 1;
                size_t tmpSize = sizeof(char) * numChar;
                char* tmpMsg = new char[tmpSize];
                strncpy_s(tmpMsg, tmpSize, ((char*)errBlob->GetBufferPointer()) + first, _TRUNCATE);
                Logger::AddLog(Logger::Channel::Channels::ERR, "%s\n", tmpMsg);
                first = i + 1;

                delete[] tmpMsg;
            }
        }
        
        return hr;
    }

    hr = CreateShader<ShaderClass>(*shaderBlob, shader, g_d3dDevice);
    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}

template<class ShaderClass>
HRESULT InitShaders() {

}

#endif // ! SHADERS_H_