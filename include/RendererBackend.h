#ifndef RENDERER_BACKEND_H_
#define RENDERER_BACKEND_H_

#include "BufferCache.h"
#include "RendererFrontend.h"
#include <functional>
#include <cstdint>
#include <d3d11_1.h>


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

	ID3D11Buffer* matPrmsCB;
	ID3D11Buffer* lightsCB;
	ID3D11Buffer* objectCB;
	ID3D11Buffer* viewCB;
	ID3D11Buffer* projCB;

	//TODO: what did I intend to do with this?
	std::hash<std::uint16_t> submeshHash;
};

extern RendererBackend rb;

#endif // !RENDERER_BACKEND_H_

