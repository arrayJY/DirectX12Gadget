//
// Created by arrayJY on 2023/04/16.
//

#pragma once

#include "../stdafx.h"

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
  void BuildDescriptors();
  void BuildResources();

  void Execute(ID3D12GraphicsCommandList* cmdList,
               ID3D12RootSignature* rootSig,
               ID3D12PipelineState* pso,
               ID3D12Resource* input,
               UINT vertexCount);

private:
  UINT VertexSize;
  DXGI_FORMAT Format;

  ComPtr<ID3D12Device> Device;
  CD3DX12_CPU_DESCRIPTOR_HANDLE InputVertexCpuSrv;
  CD3DX12_CPU_DESCRIPTOR_HANDLE OutputVertexCpuUav;
  CD3DX12_GPU_DESCRIPTOR_HANDLE InputVertexGpuSrv;
  CD3DX12_GPU_DESCRIPTOR_HANDLE OutputVertexGpuUav;

  ComPtr<ID3D12Resource> InputVertexBuffer = nullptr;
  ComPtr<ID3D12Resource> InputVertexUploadBuffer = nullptr;
  ComPtr<ID3D12Resource> OutputVertexBuffer = nullptr;
};
