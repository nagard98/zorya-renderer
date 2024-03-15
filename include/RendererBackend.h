#ifndef RENDERER_BACKEND_H_
#define RENDERER_BACKEND_H_

#include "BufferCache.h"
#include "RendererFrontend.h"
#include "Lights.h"
#include "RenderDevice.h"

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

struct GBuffer {
	enum :int {
		ALBEDO,
		NORMAL,
		ROUGH_MET,
		SIZE
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
	HRESULT Init(bool reset = false);
	void RenderView(const ViewDesc& viewDesc);

	ID3DUserDefinedAnnotation* annot;
	D3D11_VIEWPORT sceneViewport;

	ID3D11Texture2D* finalRenderTargetTex;
	ID3D11RenderTargetView* finalRenderTargetView;
	ID3D11ShaderResourceView* finalRenderTargetSRV;

	ID3D11Texture2D* depthTex;
	ID3D11DepthStencilView* depthDSV;
	ID3D11ShaderResourceView* depthSRV;

private:
	void RenderShadowMaps(const ViewDesc& viewDesc, DirShadowCB& dirShadowCB, OmniDirShadowCB& cbOmniDirShad);

	ID3D11Buffer* matPrmsCB;
	ID3D11Buffer* lightsCB;
	ID3D11Buffer* objectCB;
	ID3D11Buffer* viewCB;
	ID3D11Buffer* projCB;
	ID3D11Buffer* dirShadCB;
	ID3D11Buffer* omniDirShadCB;

	ID3D11Buffer* invMatCB;

	D3D11_VIEWPORT sm_viewport;

	//TODO:Thickness map here is temporary; move to material
	ShaderTexture2D thicknessMapSRV;

	ID3D11Texture2D* GBuffer[GBuffer::SIZE];
	ID3D11RenderTargetView* GBufferRTV[GBuffer::SIZE];
	ID3D11ShaderResourceView* GBufferSRV[GBuffer::SIZE];

	ID3D11Texture2D* ambientMap;
	ID3D11RenderTargetView* ambientRTV;
	ID3D11ShaderResourceView* ambientSRV;

	ID3D11Texture2D* shadowMap;
	ID3D11ShaderResourceView* shadowMapSRV;
	ID3D11DepthStencilView* shadowMapDSV;

	ID3D11Texture2D* spotShadowMap;
	ID3D11ShaderResourceView* spotShadowMapSRV;
	ID3D11DepthStencilView* spotShadowMapDSV;

	ID3D11Texture2D* shadowCubeMap;
	ID3D11ShaderResourceView* shadowCubeMapSRV;
	ID3D11DepthStencilView* shadowCubeMapDSV[6 * 2];

	ID3D11Texture2D* skinMaps[5];
	ID3D11RenderTargetView* skinRT[5];
	ID3D11ShaderResourceView* skinSRV[5];

	//TODO: what did I intend to do with this?
	//std::hash<std::uint16_t> submeshHash;
};

extern RendererBackend rb;

#endif // !RENDERER_BACKEND_H_

