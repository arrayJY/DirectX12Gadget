//
// Created by arrayJY on 2023/02/11.
//

#pragma once

#include "../dx_utils.h"
#include "../math_helper.h"
#include "../upload_buffer.h"

struct ObjectConstants {
  DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
};

struct PassConstants {
  DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
  DirectX::XMFLOAT3 EyePosW = {0.0f, 0.0f, 0.0f};
  float cbPerObjectPad1 = 0.0f;
  DirectX::XMFLOAT2 RenderTargetSize = {0.0f, 0.0f};
  DirectX::XMFLOAT2 InvRenderTargetSize = {0.0f, 0.0f};
  float NearZ = 0.0f;
  float FarZ = 0.0f;
  float TotalTime = 0.0f;
  float DeltaTime = 0.0f;
};

struct PBRMaterialConstants {
  DirectX::XMFLOAT4 Albedo = {1.0f, 1.0f, 1.0f, 1.0f};
  DirectX::XMFLOAT3 FresnelR0 = {0.01f, 0.01f, 0.01f};
  float Rougness = 0.25f;
  DirectX::XMFLOAT4X4 TransformMatrix = MathHelper::Identity4x4();
};

struct Vertex {
  DirectX::XMFLOAT3 Pos;
  DirectX::XMFLOAT4 Color;
};

class FrameResource {
public:
  FrameResource(ID3D12Device *device, UINT passCount,
                UINT objectCount, UINT materialCount);
  FrameResource(const FrameResource &) = delete;
  FrameResource &operator=(const FrameResource &) = delete;

  ComPtr<ID3D12CommandAllocator> CommandAllocator = nullptr;
  std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectConstantsBuffer =
      nullptr;
  std::unique_ptr<UploadBuffer<PassConstants>> PassConstantsBuffer = nullptr;
  std::unique_ptr<UploadBuffer<PBRMaterialConstants>>
      PBRMaterialConstantsBuffer = nullptr;

  UINT64 Fence = 0;
};