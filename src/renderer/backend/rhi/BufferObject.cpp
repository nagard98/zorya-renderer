#include "BufferObject.h"
#include <cassert>

VertexBuffer::VertexBuffer()
{
	buffer = nullptr;
	size = 0;
}

VertexBuffer::~VertexBuffer()
{
}

void VertexBuffer::Init(ID3D11Buffer* vBuf, int bufSize)
{
	assert(vBuf != nullptr);
	buffer.Attach(vBuf);
	size = bufSize;
}

IndexBuffer::IndexBuffer()
{
	buffer = nullptr;
	size = 0;
}

IndexBuffer::~IndexBuffer()
{
}

void IndexBuffer::Init(ID3D11Buffer* iBuf, int bufSize)
{
	assert(iBuf != nullptr);
	buffer.Attach(iBuf);
	size = bufSize;
}
