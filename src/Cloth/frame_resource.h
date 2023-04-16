//
// Created by arrayJY on 2023/04/16.
//

#pragma once
#include "../math_helper.h"
#include "../stdafx.h"
#include "../upload_buffer.h"

struct VertexInput {
  UINT32 vertexIndex;
};

struct Vertex {
  XMFLOAT3 position;
  XMFLOAT3 normal;
};

struct ObjectConstants {
  DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
  int MaterialIndex = -1;
};

struct PassConstants {
  DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
  DirectX::XMFLOAT3 EyePosW = {0.0f, 0.0f, 0.0f};
  DirectX::XMFLOAT2 RenderTargetSize = {0.0f, 0.0f};
  DirectX::XMFLOAT2 InvRenderTargetSize = {0.0f, 0.0f};
  float NearZ = 0.0f;
  float FarZ = 0.0f;
  float TotalTime = 0.0f;
  float DeltaTime = 0.0f;
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

  UINT64 Fence = 0;
};