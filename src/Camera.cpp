#include "Camera.h"


void Camera::rotate(float aroundX, float aroundY, float aroundZ) {
	_viewMatrix *= dx::XMMatrixRotationRollPitchYaw(aroundX, aroundY, aroundZ);
}

dx::XMMATRIX Camera::getViewMatrix() const {
	return dx::XMMatrixTranspose(_viewMatrix);
}

dx::XMMATRIX Camera::getProjMatrix() const {
	return dx::XMMatrixTranspose(_projMatrix);
}