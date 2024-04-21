#include "Camera.h"

namespace zorya
{
	void Camera::rotate(float around_x, float around_y, float around_z)
	{
		//dx::XMMATRIX rotFrontAxis = dx::XMMatrixRotationAxis(_camDir, aroundZ);
		dx::XMMATRIX rot_around_up_axis = dx::XMMatrixRotationAxis(dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), around_y);
		dx::XMMATRIX rot_around_right_axis = dx::XMMatrixRotationAxis(dx::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), around_x);

		m_rotation_mat = dx::XMMatrixMultiply(rot_around_up_axis, rot_around_right_axis);// dx::XMMatrixMultiply((dx::XMMatrixIdentity())/*rotFrontAxis*/,

		m_cam_up = dx::XMVector4Transform(m_cam_up, rot_around_right_axis);
		m_cam_right = dx::XMVector4Transform(m_cam_right, rot_around_up_axis);
		m_cam_dir = dx::XMVector3Cross(m_cam_right, m_cam_up);
		//_camDir = dx::XMVector4Transform(_camDir, _rotMat);
		//_camPos = dx::XMVector4Transform(_camPos, _rotMat);
	}

	void rotate_first_person()
	{
		//dx::XMMATRIX rotUpAxis = dx::XMMatrixRotationAxis(dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), aroundY);
		//dx::XMMATRIX rotRightAxis = dx::XMMatrixRotationAxis(_camRight, aroundX);

		//_rotMat = dx::XMMatrixMultiply(rotUpAxis, rotRightAxis);

		//_camDir = dx::XMVector4Transform(_camDir, _rotMat);
	}

	//TODO: use correct name; here i move also the camera posisition because we are rotating around origin and not camera position
	void Camera::rotate_around_focus_point(float around_x, float around_y)
	{

		dx::XMMATRIX rot_around_up_axis = dx::XMMatrixRotationAxis(m_cam_up, around_y);
		dx::XMMATRIX rot_around_right_axis = dx::XMMatrixRotationAxis(m_cam_right, around_x);

		m_rotation_mat = dx::XMMatrixMultiply(rot_around_up_axis, rot_around_right_axis);

		//_referenceUp = dx::XMVector4Transform(_referenceUp, rotRightAxis);
		////_camRight = dx::XMVector4Transform(_camRight, rotUpAxis);
		//_camDir = dx::XMVector4Transform(_camDir, _rotMat);
		////_camDir = dx::XMVector3Cross(_camRight, _camUp);


		m_cam_pos = dx::XMVector4Transform(m_cam_pos, m_rotation_mat);
		m_cam_dir = dx::XMVector4Normalize(dx::XMVectorNegate(m_cam_pos));


		m_cam_right = dx::XMVector3Cross(dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), m_cam_dir);
		//_camRight = dx::XMVector4Normalize(_camRight);
		//_camUp = dx::XMVector4Normalize(_camUp);

	}

	void Camera::translate_along_cam_axis(float x, float y, float z)
	{
		dx::XMVECTOR translation = dx::XMVectorMultiply(m_cam_right, dx::XMVectorSet(x, x, x, 0.0f));
		translation = dx::XMVectorAdd(translation, dx::XMVectorMultiply(m_cam_dir, dx::XMVectorSet(z, z, z, 0.0f)));
		translation = dx::XMVectorAdd(translation, dx::XMVectorMultiply(m_cam_up, dx::XMVectorSet(y, y, y, 0.0f)));
		m_cam_pos = dx::XMVectorAdd(m_cam_pos, translation);
	}

	dx::XMMATRIX Camera::get_view_matrix() const
	{
		return dx::XMMatrixLookToLH(m_cam_pos, m_cam_dir, m_cam_up);
	}

	dx::XMMATRIX Camera::get_proj_matrix() const
	{
		return m_proj_matrix;
	}

	dx::XMMATRIX Camera::get_view_matrix_transposed() const
	{
		return dx::XMMatrixTranspose(get_view_matrix());
	}

	dx::XMMATRIX Camera::get_proj_matrix_transposed() const
	{
		return dx::XMMatrixTranspose(m_proj_matrix);
	}

	dx::XMMATRIX Camera::get_rotation_matrix() const
	{
		return dx::XMMatrixLookToLH(dx::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), m_cam_dir, m_cam_up);
	}
}