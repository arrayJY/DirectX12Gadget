//
// Created by arrayJY on 2023/02/08.
//

#pragma once
#include "math_helper.h"
#include "stdafx.h"

class DXUtils {
public:
  static int CalcConstantBufferSize(int byteSize);
  static ComPtr<ID3DBlob> CompileShader(const std::wstring &fileName,
                                        const D3D_SHADER_MACRO *defines,
                                        const std::string &entryPoint,
                                        const std::string &target);
  static ComPtr<ID3D12Resource>
  CreateDefaultBuffer(ID3D12Device *device,
                      ID3D12GraphicsCommandList *commandList,
                      const void *initData, UINT64 byteSize,
                      ComPtr<ID3D12Resource> &uploadBuffer);
};

struct SubmeshGeometry {
  UINT IndexCount = 0;
  UINT StartIndexLocation = 0;
  INT BaseVertexLocation = 0;
  DirectX::BoundingBox Bounds;
};

struct MeshGeometry {
  std::string Name;

  ComPtr<ID3DBlob> VertexCPUBuffer = nullptr;
  ComPtr<ID3DBlob> IndexCPUBuffer = nullptr;
  ComPtr<ID3D12Resource> VertexGPUBuffer = nullptr;
  ComPtr<ID3D12Resource> IndexGPUBuffer = nullptr;

  ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
  ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

  UINT VertexByteStride = 0;
  UINT VertexBufferByteSize = 0;
  DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
  UINT IndexBufferByteSize = 0;

  std::unordered_map<std::string, SubmeshGeometry> DrawArgs;

  D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const;
  D3D12_INDEX_BUFFER_VIEW IndexBufferView() const;
  void DisposeUploaders();
};

struct PBRMaterial {
  std::string Name;

  int MatCBIndex = -1;
  int DiffuseSrvHeapIndex = -1;

  int NumFrameDirty = FrameResourceCount;

  DirectX::XMFLOAT4 Albedo = {1.0f, 1.0f, 1.0f, 1.0f};
  DirectX::XMFLOAT3 FresnelR0 = {0.01f, 0.01f, 0.01f};
  float Roughness = 0.25f;
  DirectX::XMFLOAT4X4 TransformMatrix = MathHelper::Identity4x4();
};

struct Light {
  DirectX::XMFLOAT3 Strength = {0.5f, 0.5f, 0.5f};
  float FalloffStart = 1.0f;
  DirectX::XMFLOAT3 Direction = {0.0f, -1.0f, 0.0f};
  float FalloffEnd = 10.0f;
  DirectX::XMFLOAT3 Position = {0.0f, 0.0f, 0.0f};
  float SpotPower = 64.0f;
};
