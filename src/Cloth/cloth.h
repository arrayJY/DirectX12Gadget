//
// Created by arrayJY on 2023/04/16.
//

#pragma once

#include "../stdafx.h"

struct Vertex
{
  DirectX::XMFLOAT3 Pos;
  DirectX::XMFLOAT3 Normal;
};

class Cloth
{
public:
  Cloth(ID3D12Device* device, UINT vertexSize, DXGI_FORMAT format);
  Cloth(const Cloth& rhs) = delete;
  Cloth operator=(const Cloth& rhs) = delete;

  ID3D12Resource* Output() const;
  void BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptor,
                        CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptor,
                        UINT descriptorSize);
  void OnResize(UINT newWidth, UINT newHeight);

  void Execute(ID3D12GraphicsCommandList* cmdList,
               ID3D12RootSignature* rootSig,
               ID3D12PipelineState* pso,
               ID3D12Resource* input,
               UINT vertexCount);

private:
  void BuildResources();
  std::vector<Vertex> Vertices;
  UINT VertexSize;
  DXGI_FORMAT Format;

  ComPtr<ID3D12Device> Device;
  CD3DX12_GPU_DESCRIPTOR_HANDLE InputVertexSrv;
  CD3DX12_GPU_DESCRIPTOR_HANDLE OutputVertexUav;

  ComPtr<ID3D12Resource> InputVertexBuffer = nullptr;
  ComPtr<ID3D12Resource> InputVertexUploadBuffer = nullptr;
  ComPtr<ID3D12Resource> OutputVertexBuffer = nullptr;
};
