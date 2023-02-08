//
// Created by arrayJY on 2023/02/08.
//

#include "render_resource.h"
#include "dx_utils.h"
#include "stdafx.h"
#include "upload_buffer.h"

Box::Box(ID3D12Device *device) : device(device) {
  auto cbvHeapDesc = D3D12_DESCRIPTOR_HEAP_DESC{
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = 1,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
      .NodeMask = 0};
  device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&cbvHeap));
}

void Box::CreateConstantBuffer() {
  UploadBuffer<ConstantObject> uploadBuffer(device, 1);
  uploadBuffer.Upload();
  UINT constantStructSize =
      DXUtils::CalcConstantBufferSize(sizeof(ConstantObject));
  auto constantBufferAddress = uploadBuffer.Resource()->GetGPUVirtualAddress();
  auto constantBufferViewDesc = D3D12_CONSTANT_BUFFER_VIEW_DESC{
      .BufferLocation = constantBufferAddress,
      .SizeInBytes = constantStructSize,
  };
  device->CreateConstantBufferView(
      &constantBufferViewDesc, cbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void Box::CreateRootSignature() {
  CD3DX12_ROOT_PARAMETER slotRootParameter[1];
  CD3DX12_DESCRIPTOR_RANGE cbvTable;

  cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
  slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

  auto rootSignDesc = CD3DX12_ROOT_SIGNATURE_DESC{
      1, slotRootParameter, 0, nullptr,
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT};

  ComPtr<ID3DBlob> serializeRootSignature = nullptr;
  ComPtr<ID3DBlob> errorBlob = nullptr;

  D3D12SerializeRootSignature(&rootSignDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                              serializeRootSignature.GetAddressOf(),
                              errorBlob.GetAddressOf());

  ThrowIfFailed(device->CreateRootSignature(
      0, serializeRootSignature->GetBufferPointer(),
      serializeRootSignature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}
