//
// Created by arrayJY on 2023/03/27.
//

#include "camera.h"

using namespace DirectX;

Camera::Camera() { SetLens(0.25f * MathHelper::PI, 1.0f, 1.0f, 1000.0f); }

DirectX::XMVECTOR Camera::GetPosition() const {
  return DirectX::XMLoadFloat3(&Position);
}

DirectX::XMFLOAT3 Camera::GetPosition3f() const { return Position; }

void Camera::SetPosition(float x, float y, float z) {
  Position = XMFLOAT3{x, y, z};
  ViewDirty = true;
}

void Camera::SetPosition(const DirectX::XMFLOAT3 &v) {
  Position = v;
  ViewDirty = true;
}

float Camera::GetNearZ() const { return NearZ; }

float Camera::GetFarZ() const { return FarZ; }

float Camera::GetAspect() const { return Aspect; }

float Camera::GetFovX() const {
  return 2.0f * atan(0.5f * GetNearWindowWidth() / NearZ);
}

float Camera::GetFovY() const { return FovY; }

float Camera::GetNearWindowWidth() const { return Aspect * NearWindowHeight; }

float Camera::GetNearWindowHeight() const { return NearWindowHeight; }

float Camera::GetFarWindowWidth() const { return Aspect * FarWindowHeight; }

float Camera::GetFarWindowHeight() const { return FarWindowHeight; }

void Camera::SetLens(float fovY, float aspect, float zn, float zf) {
  fovY = fovY;
  Aspect = aspect;
  NearZ = zn;
  FarZ = zf;

  NearWindowHeight = 2.0f * NearZ * tanf(0.5f * FovY);
  FarWindowHeight = 2.0f * FarZ * tanf(0.5f * FovY);

  XMMATRIX P = XMMatrixPerspectiveFovLH(FovY, Aspect, NearZ, FarZ);
  XMStoreFloat4x4(&Proj, P);
}

void Camera::LookAt(DirectX::XMVECTOR pos, DirectX::XMVECTOR target,
                    DirectX::XMVECTOR worldUp) {
  XMVECTOR F =
      DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(target, pos));
  XMVECTOR R = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(worldUp, F));
  XMVECTOR U = DirectX::XMVector3Cross(F, R);

  XMStoreFloat3(&Position, pos);
  XMStoreFloat3(&Forward, F);
  XMStoreFloat3(&Right, R);
  XMStoreFloat3(&Up, U);

  ViewDirty = true;
}

void Camera::LookAt(const DirectX::XMFLOAT3 &pos,
                    const DirectX::XMFLOAT3 &target,
                    const DirectX::XMFLOAT3 &up) {
  XMVECTOR P = XMLoadFloat3(&pos);
  XMVECTOR T = XMLoadFloat3(&target);
  XMVECTOR U = XMLoadFloat3(&up);

  LookAt(P, T, U);

  ViewDirty = true;
}

DirectX::XMMATRIX Camera::GetView() const {
  return DirectX::XMLoadFloat4x4(&View);
}

DirectX::XMMATRIX Camera::GetProj() const {
  return DirectX::XMLoadFloat4x4(&Proj);
}

DirectX::XMFLOAT4X4 Camera::GetView4x4f() const { return View; }

DirectX::XMFLOAT4X4 Camera::GetProj4x4f() const { return Proj; }

void Camera::Strafe(float d) {
  XMVECTOR s = DirectX::XMVectorReplicate(d);
  XMVECTOR r = XMLoadFloat3(&Right);
  XMVECTOR p = XMLoadFloat3(&Position);
  XMStoreFloat3(&Position, DirectX::XMVectorMultiplyAdd(s, r, p));

  ViewDirty = true;
}

void Camera::Walk(float d) {
  XMVECTOR s = DirectX::XMVectorReplicate(d);
  XMVECTOR f = XMLoadFloat3(&Forward);
  XMVECTOR p = XMLoadFloat3(&Position);
  XMStoreFloat3(&Position, DirectX::XMVectorMultiplyAdd(s, f, p));

  ViewDirty = true;
}

void Camera::Pitch(float angle) {
  XMMATRIX R = DirectX::XMMatrixRotationAxis(XMLoadFloat3(&Right), angle);

  XMStoreFloat3(&Up, XMVector3TransformNormal(XMLoadFloat3(&Up), R));
  XMStoreFloat3(&Forward, XMVector3TransformNormal(XMLoadFloat3(&Forward), R));

  ViewDirty = true;
}

void Camera::RotateY(float angle) {
  XMMATRIX R = DirectX::XMMatrixRotationY(angle);

  XMStoreFloat3(&Right, XMVector3TransformNormal(XMLoadFloat3(&Right), R));
  XMStoreFloat3(&Up, XMVector3TransformNormal(XMLoadFloat3(&Up), R));
  XMStoreFloat3(&Forward, XMVector3TransformNormal(XMLoadFloat3(&Forward), R));

  ViewDirty = true;
}

void Camera::UpdateViewMatrix() {
  if (!ViewDirty) {
    return;
  }
  XMVECTOR R = XMLoadFloat3(&Right);
  XMVECTOR U = XMLoadFloat3(&Up);
  XMVECTOR L = XMLoadFloat3(&Forward);
  XMVECTOR P = XMLoadFloat3(&Position);

  // Keep camera's axes orthogonal to each other and of unit length.
  L = XMVector3Normalize(L);
  U = XMVector3Normalize(XMVector3Cross(L, R));

  // U, L already ortho-normal, so no need to normalize cross product.
  R = XMVector3Cross(U, L);

  // Fill in the view matrix entries.
  float x = -XMVectorGetX(XMVector3Dot(P, R));
  float y = -XMVectorGetX(XMVector3Dot(P, U));
  float z = -XMVectorGetX(XMVector3Dot(P, L));

  XMStoreFloat3(&Right, R);
  XMStoreFloat3(&Up, U);
  XMStoreFloat3(&Forward, L);

  View(0, 0) = Right.x;
  View(1, 0) = Right.y;
  View(2, 0) = Right.z;
  View(3, 0) = x;

  View(0, 1) = Up.x;
  View(1, 1) = Up.y;
  View(2, 1) = Up.z;
  View(3, 1) = y;

  View(0, 2) = Forward.x;
  View(1, 2) = Forward.y;
  View(2, 2) = Forward.z;
  View(3, 2) = z;

  View(0, 3) = 0.0f;
  View(1, 3) = 0.0f;
  View(2, 3) = 0.0f;
  View(3, 3) = 1.0f;

  ViewDirty = false;
}
