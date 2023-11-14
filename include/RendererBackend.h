#ifndef RENDERER_BACKEND_H_
#define RENDERER_BACKEND_H_

#include "BufferCache.h"
#include "RendererFrontend.h"
#include <functional>
#include <cstdint>
#include <d3d11_1.h>


class RendererBackend {

public:
	RendererBackend();
	~RendererBackend();

	void Init();

	void RenderView(const ViewDesc& viewDesc, float smootness);

	ID3D11Buffer* matPrmsCB;
	//TODO: what did I intend to do with this?
	std::hash<std::uint16_t> submeshHash;
};

#endif // !RENDERER_BACKEND_H_

