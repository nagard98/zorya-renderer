#ifndef CAMERA_H_
#define CAMERA_H_

#include <DirectXMath.h>

namespace zorya
{
	namespace dx = DirectX;

	class Camera
	{

	public:
		Camera() {};

		Camera(const dx::XMVECTOR cam_pos, const dx::XMVECTOR cam_dir, const dx::XMVECTOR cam_up, float fov, float aspect_ratio, float near_z, float far_z)
		{
			m_cam_dir = cam_dir;
			m_cam_up = m_reference_up = cam_up;
			m_cam_pos = cam_pos;
			m_cam_right = dx::XMVector3Cross(m_cam_up, m_cam_dir);

			//rotate(0.0f, 0.0f, 0.0f);

			m_fov = fov;
			m_aspect_ratio = aspect_ratio;
			m_near_z = near_z;
			m_far_z = far_z;
			m_proj_matrix = dx::XMMatrixPerspectiveFovLH(m_fov, m_aspect_ratio, m_near_z, m_far_z);

		};

		~Camera() {}

		dx::XMMATRIX get_view_matrix() const;
		dx::XMMATRIX get_proj_matrix() const;

		dx::XMMATRIX get_view_matrix_transposed() const;
		dx::XMMATRIX get_proj_matrix_transposed() const;

		dx::XMMATRIX get_rotation_matrix() const;

		void rotate(float around_x, float around_y, float around_z);
		void rotate_around_focus_point(float around_x, float around_y);
		void translate_along_cam_axis(float x, float y, float z);

	private:

		dx::XMMATRIX m_proj_matrix;
		dx::XMVECTOR m_cam_dir, m_cam_up, m_cam_right, m_cam_pos, m_reference_up;
		dx::XMMATRIX m_rotation_mat;
		//dx::XMMATRIX _viewMatrix;

		float m_fov;
		float m_aspect_ratio;
		float m_near_z, m_far_z;

	};
}
#endif // !CAMERA_H_