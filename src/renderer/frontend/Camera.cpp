#include "Camera.h"


void Camera::rotateAroundCamAxis(float aroundX, float aroundY, float aroundZ) {
	dx::XMMATRIX rotFrontAxis = dx::XMMatrixRotationAxis(_camDir, aroundZ);
	dx::XMMATRIX rotUpAxis = dx::XMMatrixRotationAxis(dx::XMVectorSet(0.0f,1.0f,0.0f,0.0f), aroundY);
	dx::XMMATRIX rotRightAxis = dx::XMMatrixRotationAxis(_camRight, aroundX);
	_rotMat = dx::XMMatrixMultiply(rotFrontAxis, dx::XMMatrixMultiply(rotUpAxis, rotRightAxis));

	_camUp = dx::XMVector4Transform(_camUp, _rotMat);
	_camDir = dx::XMVector4Transform(_camDir, _rotMat);
	_camRight = dx::XMVector4Transform(_camRight, _rotMat);
	_camPos = dx::XMVector4Transform(_camPos, _rotMat);
}

void Camera::translateAlongCamAxis(float x, float y, float z)
{
	dx::XMVECTOR transVec = dx::XMVectorMultiply(_camRight, dx::XMVectorSet(x, x, x, 0.0f));
	transVec = dx::XMVectorAdd(transVec, dx::XMVectorMultiply(_camDir, dx::XMVectorSet(z, z, z, 0.0f)));
	transVec = dx::XMVectorAdd(transVec, dx::XMVectorMultiply(_camUp, dx::XMVectorSet(y, y, y, 0.0f)));
	_camPos = dx::XMVectorAdd(_camPos, transVec);
}

dx::XMMATRIX Camera::getViewMatrix() const{
	return dx::XMMatrixLookToLH(_camPos, _camDir, _camUp);
}

dx::XMMATRIX Camera::getProjMatrix() const {
	return _projMatrix;
}

dx::XMMATRIX Camera::getViewMatrixTransposed() const {
	return dx::XMMatrixTranspose(getViewMatrix());
}

dx::XMMATRIX Camera::getProjMatrixTransposed() const {
	return dx::XMMatrixTranspose(_projMatrix);
}

dx::XMMATRIX Camera::getRotationMatrix() const {
	return dx::XMMatrixLookToLH(dx::XMVectorSet(0.0f,0.0f,0.0f,1.0f), _camDir, _camUp);
}