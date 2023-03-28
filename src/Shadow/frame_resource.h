//
// Created by arrayJY on 2023/02/11.
//

#pragma once

#include "../dx_utils.h"
#include "../math_helper.h"
#include "../upload_buffer.h"

struct ObjectConstants {
  DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
  int MaterialIndex = -1;
};

constexpr auto MaxLights = 16;

struct Light {
  DirectX::XMFLOAT3 Color = {1.0f, 1.0f, 1.0f};
  float Intensity = 0.0f;
  DirectX::XMFLOAT3 Direction = {0.0f, -1.0f, 0.0f};
  float _padding = 0.0f;
};

struct PassConstants {
  DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 ShadowTransform = MathHelper::Identity4x4();
  DirectX::XMFLOAT3 EyePosW = {0.0f, 0.0f, 0.0f};
  float cbPerObjectPad1 = 0.0f;
  DirectX::XMFLOAT2 RenderTargetSize = {0.0f, 0.0f};
  DirectX::XMFLOAT2 InvRenderTargetSize = {0.0f, 0.0f};
  float NearZ = 0.0f;
  float FarZ = 0.0f;
  float TotalTime = 0.0f;
  float DeltaTime = 0.0f;

  Light Lights[MaxLights];
};

struct MaterialConstants {
  DirectX::XMFLOAT4 Albedo = {1.0f, 1.0f, 1.0f, 1.0f};
  DirectX::XMFLOAT3 FresnelR0 = {0.01f, 0.01f, 0.01f};
  float Roughness = 0.25f;
  float Metallic = 0.0f;
  DirectX::XMFLOAT4X4 TransformMatrix = MathHelper::Identity4x4();
  UINT DiffuseMapIndex = 0;
  UINT NormalMapIndex = 0;
  UINT MaterialPad1;
  UINT MaterialPad2;
};

struct AreaLight {
  DirectX::XMFLOAT3 Color = {0.0f, 0.0f, 0.0f};
  float Intensity = 0.0f;
  DirectX::XMFLOAT3X4 Verties = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
};

struct Vertex {
  DirectX::XMFLOAT3 Pos;
  DirectX::XMFLOAT3 Normal;
  DirectX::XMFLOAT2 TexC;
  DirectX::XMFLOAT3 TangentU;
};

struct Material {
  std::string Name;

  int MatCBIndex = -1;
  int DiffuseSrvHeapIndex = -1;
  int NormalSrvHeapIndex = -1;

  int NumFrameDirty = FrameResourceCount;

  DirectX::XMFLOAT4 Albedo = {1.0f, 1.0f, 1.0f, 1.0f};
  DirectX::XMFLOAT3 FresnelR0 = {0.0f, 0.0f, 0.0f};

  float Roughness = 0.25f;
  float Metallic = 0.0f;
  DirectX::XMFLOAT4X4 TransformMatrix = MathHelper::Identity4x4();
};

class FrameResource {
public:
  FrameResource(ID3D12Device *device, UINT passCount, UINT objectCount,
                UINT materialCount);
  FrameResource(const FrameResource &) = delete;
  FrameResource &operator=(const FrameResource &) = delete;

  ComPtr<ID3D12CommandAllocator> CommandAllocator = nullptr;
  std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectConstantsBuffer =
      nullptr;
  std::unique_ptr<UploadBuffer<PassConstants>> PassConstantsBuffer = nullptr;
  std::unique_ptr<UploadBuffer<MaterialConstants>> MaterialConstantsBuffer =
      nullptr;

  UINT64 Fence = 0;
};