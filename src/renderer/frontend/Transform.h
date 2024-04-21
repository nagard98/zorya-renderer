#ifndef TRANSFORM_H_
#define TRANSFORM_H_

#include <DirectXMath.h>

#define IDENTITY_TRANSFORM Transform_t{ dx::XMFLOAT3(0.0f,0.0f,0.0f), dx::XMFLOAT3(0.0f,0.0f,0.0f), dx::XMFLOAT3(1.0f,1.0f,1.0f) }

namespace zorya
{
	namespace dx = DirectX;

	struct Transform_t
	{
		dx::XMFLOAT3 pos;
		dx::XMFLOAT3 rot;
		dx::XMFLOAT3 scal;
	};

	dx::XMMATRIX mult(const Transform_t& a, const Transform_t& b);
	dx::XMMATRIX mult(const Transform_t& a, const dx::XMMATRIX& b);
	dx::XMMATRIX mult(const dx::XMMATRIX& a, const Transform_t& b);

}

#endif // !TRANSFORM_H_