#include "Transform.h"

namespace zorya
{
	dx::XMMATRIX mult(const Transform_t& a, const Transform_t& b)
	{
		dx::XMMATRIX rotation_transform_a = dx::XMMatrixMultiply(
			dx::XMMatrixRotationX(a.rot.x),
			dx::XMMatrixMultiply(dx::XMMatrixRotationY(a.rot.y), dx::XMMatrixRotationZ(a.rot.z)));
		dx::XMMATRIX matrix_transform_a = dx::XMMatrixMultiply(dx::XMMatrixScaling(a.scal.x, a.scal.y, a.scal.z), rotation_transform_a);
		matrix_transform_a = dx::XMMatrixMultiply(matrix_transform_a, dx::XMMatrixTranslation(a.pos.x, a.pos.y, a.pos.z));

		dx::XMMATRIX rotation_transform_b = dx::XMMatrixMultiply(
			dx::XMMatrixRotationX(b.rot.x),
			dx::XMMatrixMultiply(dx::XMMatrixRotationY(b.rot.y), dx::XMMatrixRotationZ(b.rot.z)));
		dx::XMMATRIX matrix_transform_b = dx::XMMatrixMultiply(dx::XMMatrixScaling(b.scal.x, b.scal.y, b.scal.z), rotation_transform_b);
		matrix_transform_b = dx::XMMatrixMultiply(matrix_transform_b, dx::XMMatrixTranslation(b.pos.x, b.pos.y, b.pos.z));

		return dx::XMMatrixMultiply(matrix_transform_a, matrix_transform_b);
	}

	dx::XMMATRIX mult(const Transform_t& a, const dx::XMMATRIX& b)
	{
		dx::XMMATRIX rotation_transform_a = dx::XMMatrixMultiply(
			dx::XMMatrixRotationX(a.rot.x),
			dx::XMMatrixMultiply(dx::XMMatrixRotationY(a.rot.y), dx::XMMatrixRotationZ(a.rot.z)));
		dx::XMMATRIX matrix_transform_a = dx::XMMatrixMultiply(dx::XMMatrixScaling(a.scal.x, a.scal.y, a.scal.z), rotation_transform_a);
		matrix_transform_a = dx::XMMatrixMultiply(matrix_transform_a, dx::XMMatrixTranslation(a.pos.x, a.pos.y, a.pos.z));

		return dx::XMMatrixMultiply(matrix_transform_a, b);
	}

	dx::XMMATRIX mult(const dx::XMMATRIX& a, const Transform_t& b)
	{
		dx::XMMATRIX rotation_transform_b = dx::XMMatrixMultiply(
			dx::XMMatrixRotationX(b.rot.x),
			dx::XMMatrixMultiply(dx::XMMatrixRotationY(b.rot.y), dx::XMMatrixRotationZ(b.rot.z)));
		dx::XMMATRIX matrix_transform_b = dx::XMMatrixMultiply(dx::XMMatrixScaling(b.scal.x, b.scal.y, b.scal.z), rotation_transform_b);
		matrix_transform_b = dx::XMMatrixMultiply(matrix_transform_b, dx::XMMatrixTranslation(b.pos.x, b.pos.y, b.pos.z));

		return dx::XMMatrixMultiply(a, matrix_transform_b);
	}

	Transform_t build_transform(aiMatrix4x4 assimp_transform)
	{
		aiVector3D scal;
		aiVector3D pos;
		aiVector3D rot;
		assimp_transform.Decompose(scal, rot, pos);

		return Transform_t{ dx::XMFLOAT3{pos.x, pos.y, pos.z}, dx::XMFLOAT3{rot.x, rot.y, rot.z}, dx::XMFLOAT3{scal.x, scal.y, scal.z} };
	}
}