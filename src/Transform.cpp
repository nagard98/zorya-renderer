#include "Transform.h"

dx::XMMATRIX mult(const Transform_t& a, const Transform_t& b) {
    dx::XMMATRIX rotTransfA = dx::XMMatrixMultiply(
        dx::XMMatrixRotationX(a.rot.x),
        dx::XMMatrixMultiply(dx::XMMatrixRotationY(a.rot.y), dx::XMMatrixRotationZ(a.rot.z)));
    dx::XMMATRIX matTransfA = dx::XMMatrixMultiply(dx::XMMatrixScaling(a.scal.x, a.scal.y, a.scal.z), rotTransfA);
    matTransfA = dx::XMMatrixMultiply(matTransfA, dx::XMMatrixTranslation(a.pos.x, a.pos.y, a.pos.z));

    dx::XMMATRIX rotTransfB = dx::XMMatrixMultiply(
        dx::XMMatrixRotationX(b.rot.x),
        dx::XMMatrixMultiply(dx::XMMatrixRotationY(b.rot.y), dx::XMMatrixRotationZ(b.rot.z)));
    dx::XMMATRIX matTransfB = dx::XMMatrixMultiply(dx::XMMatrixScaling(b.scal.x, b.scal.y, b.scal.z), rotTransfB);
    matTransfB = dx::XMMatrixMultiply(matTransfB, dx::XMMatrixTranslation(b.pos.x, b.pos.y, b.pos.z));

    return dx::XMMatrixMultiply(matTransfA, matTransfB);
}

dx::XMMATRIX mult(const Transform_t& a, const dx::XMMATRIX& b) {
    dx::XMMATRIX rotTransfA = dx::XMMatrixMultiply(
        dx::XMMatrixRotationX(a.rot.x),
        dx::XMMatrixMultiply(dx::XMMatrixRotationY(a.rot.y), dx::XMMatrixRotationZ(a.rot.z)));
    dx::XMMATRIX matTransfA = dx::XMMatrixMultiply(dx::XMMatrixScaling(a.scal.x, a.scal.y, a.scal.z), rotTransfA);
    matTransfA = dx::XMMatrixMultiply(matTransfA, dx::XMMatrixTranslation(a.pos.x, a.pos.y, a.pos.z));

    return dx::XMMatrixMultiply(matTransfA, b);
}

dx::XMMATRIX mult(const dx::XMMATRIX& a, const Transform_t& b) {
    dx::XMMATRIX rotTransfB = dx::XMMatrixMultiply(
        dx::XMMatrixRotationX(b.rot.x),
        dx::XMMatrixMultiply(dx::XMMatrixRotationY(b.rot.y), dx::XMMatrixRotationZ(b.rot.z)));
    dx::XMMATRIX matTransfB = dx::XMMatrixMultiply(dx::XMMatrixScaling(b.scal.x, b.scal.y, b.scal.z), rotTransfB);
    matTransfB = dx::XMMatrixMultiply(matTransfB, dx::XMMatrixTranslation(b.pos.x, b.pos.y, b.pos.z));

    return dx::XMMatrixMultiply(a, matTransfB);
}