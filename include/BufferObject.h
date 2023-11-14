#ifndef BUFFER_OBJECT_H_
#define BUFFER_OBJECT_H_

#include <d3d11_1.h>
#include <wrl/client.h>


namespace wrl = Microsoft::WRL;

class VertexBuffer 
{

public:
	VertexBuffer();
	~VertexBuffer();

	void Init(ID3D11Buffer* vBuf, int bufSize);

	wrl::ComPtr<ID3D11Buffer> buffer;
	int size;

};

class IndexBuffer 
{

public:
	IndexBuffer();
	~IndexBuffer();

	void Init(ID3D11Buffer* iBuf, int bufSize);

	wrl::ComPtr<ID3D11Buffer> buffer;
	int size;

};

#endif