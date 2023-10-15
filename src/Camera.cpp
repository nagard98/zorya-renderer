#include "../include/Camera.h"

void Camera::rotate(float aroundX, float aroundY, float aroundZ) {
	_viewMatrix *= dx::XMMatrixRotationRollPitchYaw(aroundX, aroundY, aroundZ);
}

dx::XMMATRIX Camera::getViewMatrix() {
	return dx::XMMatrixTranspose(_viewMatrix);
}

dx::XMMATRIX Camera::getProjMatrix() {
	return dx::XMMatrixTranspose(_projMatrix);
}