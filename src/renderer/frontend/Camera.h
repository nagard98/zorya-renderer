#ifndef CAMERA_H_
#define CAMERA_H_

#include <DirectXMath.h>

namespace dx = DirectX;

class Camera {

public:
	Camera() {};

	Camera(const dx::XMVECTOR camPos, const dx::XMVECTOR camDir, const dx::XMVECTOR camUp, float fov, float aspectRatio, float nearZ, float farZ){
		_camDir = camDir;
		_camUp = _referenceUp = camUp;
		_camPos = camPos;
		_camRight = dx::XMVector3Cross(_camUp, _camDir);

		//rotate(0.0f, 0.0f, 0.0f);

		_fov = fov;
		_aspectRatio = aspectRatio;
		_nearZ = nearZ;
		_farZ = farZ;
		_projMatrix = dx::XMMatrixPerspectiveFovLH(_fov, _aspectRatio, _nearZ, _farZ);

	};

	~Camera() {}

	dx::XMMATRIX getViewMatrix() const;
	dx::XMMATRIX getProjMatrix() const;

	dx::XMMATRIX getViewMatrixTransposed() const;
	dx::XMMATRIX getProjMatrixTransposed() const;

	dx::XMMATRIX getRotationMatrix() const;

	void rotate(float aroundX, float aroundY, float aroundZ);
	void rotateAroundFocusPoint(float aroundX, float aroundY);
	void translateAlongCamAxis(float x, float y, float z);

private:

	dx::XMMATRIX _projMatrix;
	dx::XMVECTOR _camDir, _camUp, _camRight, _camPos, _referenceUp;
	dx::XMMATRIX _rotMat;
	//dx::XMMATRIX _viewMatrix;

	float _fov;
	float _aspectRatio;
	float _nearZ, _farZ;

};


#endif // !CAMERA_H_