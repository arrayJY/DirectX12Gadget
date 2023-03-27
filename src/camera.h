//
// Created by arrayJY on 2023/03/24.
//

#pragma once

#include "dx_utils.h"

class Camera {
public:
  Camera();

  DirectX::XMVECTOR GetPosition() const;
  DirectX::XMFLOAT3 GetPosition3f() const;

  void SetPosition(float x, float y, float z);
  void SetPosition(const DirectX::XMFLOAT3 &v);

  float GetNearZ() const;
  float GetFarZ() const;
  float GetAspect() const;
  float GetFovX() const;
  float GetFovY() const;

  float GetNearWindowWidth() const;
  float GetNearWindowHeight() const;
  float GetFarWindowWidth() const;
  float GetFarWindowHeight() const;

  void SetLens(float fovY, float aspect, float zn, float zf);

  void LookAt(DirectX::XMVECTOR pos, DirectX::XMVECTOR target,
              DirectX::XMVECTOR worldUp);
  void LookAt(const DirectX::XMFLOAT3 &pos, const DirectX::XMFLOAT3 &target,
              const DirectX::XMFLOAT3 &up);
  // Get View/Proj matrices.
  DirectX::XMMATRIX GetView() const;
  DirectX::XMMATRIX GetProj() const;

  DirectX::XMFLOAT4X4 GetView4x4f() const;
  DirectX::XMFLOAT4X4 GetProj4x4f() const;

  // Strafe/Walk the camera a distance d.
  void Strafe(float d);
  void Walk(float d);

  // Rotate the camera.
  void Pitch(float angle);
  void RotateY(float angle);

  // After modifying camera position/orientation, call to rebuild the view
  // matrix.
  void UpdateViewMatrix();

private:
  DirectX::XMFLOAT3 Position = {0.0f, 0.0f, 0.0f};
  DirectX::XMFLOAT3 Right = {1.0f, 0.0f, 0.0f};
  DirectX::XMFLOAT3 Up = {0.0f, 1.0f, 0.0f};
  DirectX::XMFLOAT3 Forward = {0.0f, 0.0f, 1.0f};

  float NearZ = 0.0f;
  float FarZ = 0.0f;
  float Aspect = 0.0f;
  float FovY = 0.0f;
  float NearWindowHeight = 0.0f;
  float FarWindowHeight = 0.0f;

  bool ViewDirty = true;

  DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
};