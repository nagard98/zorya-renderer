#ifndef RENDERER_BACKEND_H_
#define RENDERER_BACKEND_H_

#include "BufferCache.h"
#include "RendererFrontend.h"
#include "Lights.h"

#include <functional>
#include <cstdint>
#include <d3d11_1.h>

struct CUBEMAP {
	enum : int {
		FACE_POSITIVE_X = 0,
		FACE_NEGATIVE_X = 1,
		FACE_POSITIVE_Y = 2,
		FACE_NEGATIVE_Y = 3,
		FACE_POSITIVE_Z = 4,
		FACE_NEGATIVE_Z = 5,
	};
};

enum ConstanBuffer {
	CB_Application,
	CB_Frame,
	CB_Object,
	CB_Light,
	NumConstantBuffers
};

struct ObjCB {
	dx::XMMATRIX worldMatrix;
};

struct ViewCB {
	dx::XMMATRIX viewMatrix;
};

struct ProjCB {
	dx::XMMATRIX projMatrix;
};


class RendererBackend {

public:
	RendererBackend();
	~RendererBackend();

	//TODO: change return type to custom wrapper
	HRESULT Init();

	void RenderView(const ViewDesc& viewDesc);
	void RenderShadowMaps(const std::vector<SubmeshInfo>& submeshesInfo, DirShadowCB& dirShadowCB, OmniDirShadowCB& cbOmniDirShad);

	ID3D11Buffer* matPrmsCB;
	ID3D11Buffer* lightsCB;
	ID3D11Buffer* objectCB;
	ID3D11Buffer* viewCB;
	ID3D11Buffer* projCB;
	ID3D11Buffer* dirShadCB;
	ID3D11Buffer* omniDirShadCB;

	D3D11_VIEWPORT sm_viewport;

	ID3D11Texture2D* shadowMap;
	ID3D11ShaderResourceView* shadowMapSRV;
	ID3D11DepthStencilView* shadowMapDSV;

	ID3D11Texture2D* spotShadowMap;
	ID3D11ShaderResourceView* spotShadowMapSRV;
	ID3D11DepthStencilView* spotShadowMapDSV;

	ID3D11Texture2D* shadowCubeMap;
	ID3D11ShaderResourceView* shadowCubeMapSRV;
	ID3D11DepthStencilView* shadowCubeMapDSV[6 * 2];

	//TODO: what did I intend to do with this?
	std::hash<std::uint16_t> submeshHash;
};

extern RendererBackend rb;

#endif // !RENDERER_BACKEND_H_

