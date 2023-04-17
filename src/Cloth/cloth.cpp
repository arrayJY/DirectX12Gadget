//
// Created by arrayJY on 2023/04/17.
//

#include "cloth.h"
#include "../dx_utils.h"
#include "frame_resource.h"

Cloth::Cloth(ID3D12Device* device, UINT vertexSize, DXGI_FORMAT format)
{
  VertexSize = vertexSize;
  Format = format;
  BuildResources();
}

ID3D12Resource* Cloth::Output() const
{
  return OutputVertexBuffer.Get();
}

void Cloth::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptor,
                             CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptor,
                             UINT descriptorSize)
{
  InputVertexCpuSrv = cpuDescriptor;
  OutputVertexCpuUav = cpuDescriptor.Offset(1, descriptorSize);
  InputVertexGpuSrv = gpuDescriptor;
  OutputVertexGpuUav = gpuDescriptor.Offset(1, descriptorSize);

  BuildDescriptors();
}

void Cloth::BuildDescriptors()
{
  auto srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC{
    .Format = Format,
    .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
    .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
    .Buffer =
      D3D12_BUFFER_SRV{
        .FirstElement = 0,
        .NumElements = (UINT)VertexSize,
        .StructureByteStride = (UINT)sizeof(Vertex),
        .Flags = D3D12_BUFFER_SRV_FLAG_NONE,
      },
  };
  auto uavDesc =
    D3D12_UNORDERED_ACCESS_VIEW_DESC{ .Format = Format,
                                      .ViewDimension =
                                        D3D12_UAV_DIMENSION_BUFFER,
                                      .Buffer = {
                                        .FirstElement = 0,
                                        .NumElements = VertexSize,
                                        .StructureByteStride =
                                          (UINT)sizeof(Vertex),
                                        .CounterOffsetInBytes = 0,
                                        .Flags = D3D12_BUFFER_UAV_FLAG_NONE,
                                      } };
  Device->CreateShaderResourceView(
    InputVertexBuffer.Get(), &srvDesc, InputVertexCpuSrv);
  Device->CreateUnorderedAccessView(
    OutputVertexBuffer.Get(), nullptr, &uavDesc, OutputVertexCpuUav);
}

void Cloth::Execute(ID3D12GraphicsCommandList* cmdList,
                    ID3D12RootSignature* rootSig,
                    ID3D12PipelineState* pso,
                    ID3D12Resource* input,
                    UINT vertexCount)
{
  cmdList->SetComputeRootSignature(rootSig);
  auto barrier =
    CD3DX12_RESOURCE_BARRIER::Transition(InputVertexBuffer.Get(),
                                         D3D12_RESOURCE_STATE_COMMON,
                                         D3D12_RESOURCE_STATE_COPY_DEST);

  // Copy input to input buffer
  cmdList->ResourceBarrier(1, &barrier);
  cmdList->CopyResource(InputVertexBuffer.Get(), input);

  barrier =
    CD3DX12_RESOURCE_BARRIER::Transition(InputVertexBuffer.Get(),
                                         D3D12_RESOURCE_STATE_COPY_DEST,
                                         D3D12_RESOURCE_STATE_GENERIC_READ);
  cmdList->ResourceBarrier(1, &barrier);

  barrier =
    CD3DX12_RESOURCE_BARRIER::Transition(OutputVertexBuffer.Get(),
                                         D3D12_RESOURCE_STATE_COMMON,
                                         D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  cmdList->ResourceBarrier(1, &barrier);

  cmdList->SetPipelineState(pso);
  cmdList->SetComputeRootDescriptorTable(0, InputVertexGpuSrv);
  cmdList->SetComputeRootDescriptorTable(1, OutputVertexGpuUav);

  UINT GroupSize = (UINT)ceilf(vertexCount / 64.0f);
  cmdList->Dispatch(GroupSize, 1, 1);
}

void Cloth::BuildResources()
{
  auto bufferDesc =
    D3D12_RESOURCE_DESC{ .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
                         .Alignment = 0,
                         .Width = VertexSize * (UINT)sizeof(Vertex),
                         .Height = 1,
                         .DepthOrArraySize = 1,
                         .MipLevels = 1,
                         .Format = DXGI_FORMAT_UNKNOWN,
                         .SampleDesc = { 1, 0 },
                         .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
                         .Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS };

  auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  ThrowIfFailed(Device->CreateCommittedResource(
    &heapProps,
    D3D12_HEAP_FLAG_NONE,
    &bufferDesc,
    D3D12_RESOURCE_STATE_COMMON,
    nullptr,
    IID_PPV_ARGS(InputVertexBuffer.GetAddressOf())));
  ThrowIfFailed(Device->CreateCommittedResource(
    &heapProps,
    D3D12_HEAP_FLAG_NONE,
    &bufferDesc,
    D3D12_RESOURCE_STATE_COMMON,
    nullptr,
    IID_PPV_ARGS(OutputVertexBuffer.GetAddressOf())));
}
