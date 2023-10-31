#include <d3d11_1.h>
#include <wrl/client.h>

namespace wrl = Microsoft::WRL;

class VertexBuffer 
{

public:
	VertexBuffer();
	~VertexBuffer();

	void Init();

	wrl::ComPtr<ID3D11Buffer> buffer;
	int size;

};

class IndexBuffer 
{

public:
	IndexBuffer();
	~IndexBuffer();

	void Init();

	wrl::ComPtr<ID3D11Buffer> buffer;
	int size;

};