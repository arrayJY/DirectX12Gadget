//
// Created by arrayJY on 2023/04/16.
//

#pragma once

#include "../stdafx.h"

class Cloth {
public:
  Cloth(ID3D12Device *device, UINT width, UINT height, UINT depth);
  Cloth(const Cloth &rhs) = delete;
  Cloth operator=(const Cloth &rhs) = delete;

  ID3D12Resource *Output() const;
  void BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor,
                        CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor,
                        UINT descriptorSize);
  void OnResize(UINT newWidth, UINT newHeight);

  void Execute(ID3D12GraphicsCommandList *cmdList, ID3D12RootSignature *rootSig,
               ID3D12PipelineState *pso, ID3D12Resource *input,
               UINT vertexCount);
};
