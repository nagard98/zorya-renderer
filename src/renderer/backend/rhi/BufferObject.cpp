#include "BufferObject.h"
#include <cassert>

namespace zorya
{
	Vertex_Buffer::Vertex_Buffer()
	{
		buffer = nullptr;
		size = 0;
	}

	Vertex_Buffer::~Vertex_Buffer()
	{}

	void Vertex_Buffer::init(ID3D11Buffer* vertex_buffer, int buffer_size)
	{
		assert(vertex_buffer != nullptr);
		//buffer.Attach(vertex_buffer);
		buffer = vertex_buffer;
		size = buffer_size;
	}

	Index_Buffer::Index_Buffer()
	{
		buffer = nullptr;
		size = 0;
	}

	Index_Buffer::~Index_Buffer()
	{}

	void Index_Buffer::init(ID3D11Buffer* index_buffer, int buffer_size)
	{
		assert(index_buffer != nullptr);
		//buffer.Attach(index_buffer);
		buffer = index_buffer;
		size = buffer_size;
	}
}
