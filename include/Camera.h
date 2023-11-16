#ifndef CAMERA_H_
#define CAMERA_H_

#include <DirectXMath.h>

namespace dx = DirectX;

class Camera {

public:
	Camera() {};

	Camera(const dx::XMVECTOR camPos, const dx::XMVECTOR camDir, const dx::XMVECTOR camUp, float fov, float aspectRatio, float nearZ, float farZ){
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

	~Camera() {
		
	}

	//TODO: why make transpose inside these functions?
	dx::XMMATRIX getViewMatrix() const;
	dx::XMMATRIX getProjMatrix() const;

	void rotate(float aroundX, float aroundY, float aroundZ);

	dx::XMMATRIX _viewMatrix;
	dx::XMVECTOR _camPos;

private:

	dx::XMMATRIX _projMatrix;
	dx::XMVECTOR _camDir, _camUp;

	float _fov;
	float _aspectRatio;
	float _nearZ, _farZ;

};


#endif // !CAMERA_H_