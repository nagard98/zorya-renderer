#include "Camera.h"


void Camera::rotate(float aroundX, float aroundY, float aroundZ)
{
	//dx::XMMATRIX rotFrontAxis = dx::XMMatrixRotationAxis(_camDir, aroundZ);
	dx::XMMATRIX rotUpAxis = dx::XMMatrixRotationAxis(dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), aroundY);
	dx::XMMATRIX rotRightAxis = dx::XMMatrixRotationAxis(dx::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), aroundX);

	_rotMat = dx::XMMatrixMultiply(rotUpAxis, rotRightAxis);// dx::XMMatrixMultiply((dx::XMMatrixIdentity())/*rotFrontAxis*/,

	_camUp = dx::XMVector4Transform(_camUp, rotRightAxis);
	_camRight = dx::XMVector4Transform(_camRight, rotUpAxis);
	_camDir = dx::XMVector3Cross(_camRight, _camUp);
	//_camDir = dx::XMVector4Transform(_camDir, _rotMat);
	//_camPos = dx::XMVector4Transform(_camPos, _rotMat);
}

void rotatefps() {
	//dx::XMMATRIX rotUpAxis = dx::XMMatrixRotationAxis(dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), aroundY);
	//dx::XMMATRIX rotRightAxis = dx::XMMatrixRotationAxis(_camRight, aroundX);

	//_rotMat = dx::XMMatrixMultiply(rotUpAxis, rotRightAxis);

	//_camDir = dx::XMVector4Transform(_camDir, _rotMat);
}

//TODO: use correct name; here i move also the camera posisition because we are rotating around origin and not camera position
void Camera::rotateAroundFocusPoint(float aroundX, float aroundY) {
	
	dx::XMMATRIX rotUpAxis = dx::XMMatrixRotationAxis(_camUp, aroundY);
	dx::XMMATRIX rotRightAxis = dx::XMMatrixRotationAxis(_camRight, aroundX);
	
	_rotMat = dx::XMMatrixMultiply(rotUpAxis, rotRightAxis);

	//_referenceUp = dx::XMVector4Transform(_referenceUp, rotRightAxis);
	////_camRight = dx::XMVector4Transform(_camRight, rotUpAxis);
	//_camDir = dx::XMVector4Transform(_camDir, _rotMat);
	////_camDir = dx::XMVector3Cross(_camRight, _camUp);


	_camPos = dx::XMVector4Transform(_camPos, _rotMat);
	_camDir = dx::XMVector4Normalize(dx::XMVectorNegate(_camPos));


	_camRight = dx::XMVector3Cross(dx::XMVectorSet(0.0f,1.0f,0.0f,0.0f), _camDir);
	//_camRight = dx::XMVector4Normalize(_camRight);
	//_camUp = dx::XMVector4Normalize(_camUp);

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