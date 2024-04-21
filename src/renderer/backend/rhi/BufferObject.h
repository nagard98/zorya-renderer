#ifndef BUFFER_OBJECT_H_
#define BUFFER_OBJECT_H_

#include <d3d11_1.h>
#include <wrl/client.h>

namespace zorya
{
	namespace wrl = Microsoft::WRL;

	class Vertex_Buffer
	{

	public:
		Vertex_Buffer();
		~Vertex_Buffer();

		void init(ID3D11Buffer* vertex_buffer, int buffer_size);

		wrl::ComPtr<ID3D11Buffer> buffer;
		int size;

	};

	class Index_Buffer
	{

	public:
		Index_Buffer();
		~Index_Buffer();

		void init(ID3D11Buffer* index_buffer, int buffer_size);

		wrl::ComPtr<ID3D11Buffer> buffer;
		int size;

	};

}
#endif