#include <DirectXMath.h>

namespace dx = DirectX;

class Camera {

public:
	Camera::Camera() {};

	Camera::Camera(const dx::XMVECTOR camPos, const dx::XMVECTOR camDir, const dx::XMVECTOR camUp, float fov, float aspectRatio, float nearZ, float farZ){
		_camPos = camPos;
		_camDir = camDir;
		_camUp = camUp;
		_viewMatrix = dx::XMMatrixLookAtLH(_camPos, _camDir, _camUp);

		_fov = fov;
		_aspectRatio = aspectRatio;
		_nearZ = nearZ;
		_farZ = farZ;
		_projMatrix = dx::XMMatrixPerspectiveFovLH(_fov, _aspectRatio, _nearZ, _farZ);
	};

	Camera::~Camera() {
		
	}

	dx::XMMATRIX Camera::getViewMatrix();
	dx::XMMATRIX Camera::getProjMatrix();

	void Camera::rotate(float aroundX, float aroundY, float aroundZ);

	dx::XMMATRIX _viewMatrix;

private:

	dx::XMMATRIX _projMatrix;
	dx::XMVECTOR _camPos, _camDir, _camUp;

	float _fov;
	float _aspectRatio;
	float _nearZ, _farZ;

};