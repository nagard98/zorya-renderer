#ifndef SHADERS_H_
#define SHADERS_H_

#include <Windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <string>
#include <cstdint>
#include <vector>

#include "editor/Logger.h"


namespace zorya
{
	class Shaders
	{
	public:
		~Shaders();

		HRESULT init();
		HRESULT build_default_shaders();
		HRESULT load_default_shaders();

		void release_all_resources();

		std::vector<ID3D11VertexShader*> m_vertex_shaders;
		std::vector<ID3D11PixelShader*> m_pixel_shaders;

		ID3D11InputLayout* m_vertex_layout;
		ID3D11InputLayout* m_simple_vertex_layout;
	};

	//extern Shaders shaders;

	//extern std::vector<ID3D11VertexShader*> vertexShaders;
	//extern std::vector<ID3D11PixelShader*> pixelShaders;
	//
	//extern D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[4];

	template<class ShaderClass>
	HRESULT create_shader(ID3DBlob* pShaderBlob, ShaderClass** pShader, ID3D11Device* d3d_device);

	template<class ShaderClass>
	std::string get_latest_profile();


	template<class ShaderClass>
	HRESULT load_shader(const std::wstring& shader_filepath, const std::string& entrypoint, ID3DBlob** shader_blob, ShaderClass** shader_class, ID3D11Device* d3d_device)
	{

		wrl::ComPtr<ID3DBlob> err_blob = nullptr;

		std::string profile = get_latest_profile<ShaderClass>();

		UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG;

		HRESULT hr = D3DCompileFromFile(shader_filepath.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, entrypoint.c_str(), profile.c_str(), flags, 0, shader_blob, err_blob.GetAddressOf());

		if (FAILED(hr))
		{
			char* err_msg = (char*)err_blob->GetBufferPointer();

			for (int i = 0, first = 0; err_msg[i] != '\0'; i++)
			{
				if (err_msg[i] == '\n')
				{
					int num_char = i - first + 1;
					size_t tmp_size = sizeof(char) * num_char;
					char* tmp_msg = new char[tmp_size];
					strncpy_s(tmp_msg, tmp_size, ((char*)err_blob->GetBufferPointer()) + first, _TRUNCATE);
					Logger::add_log(Logger::Channel::Channels::ERR, "%s\n", tmp_msg);
					first = i + 1;

					delete[] tmp_msg;
				}
			}

			return hr;
		}

		hr = create_shader<ShaderClass>(*shader_blob, shader_class, d3d_device);
		if (FAILED(hr))
		{
			return hr;
		}

		return S_OK;
	}

	template<class ShaderClass>
	HRESULT init_shaders()
	{

	}
}

#endif // ! SHADERS_H_