//
// Created by arrayJY on 2023/02/08.
//

#include "dx_utils.h"

int DXUtils::CalcConstantBufferSize(int byteSize) {
  return (byteSize + 0xFF) & (~0xFF);
}

ComPtr<ID3DBlob> DXUtils::CompileShader(const std::wstring &fileName,
                                        const D3D_SHADER_MACRO *defines,
                                        const std::string &entryPoint,
                                        const std::string &target) {
  UINT compileFlags = 0;
#if DEBUG
  compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

  ComPtr<ID3DBlob> byteCode = nullptr;
  ComPtr<ID3DBlob> errors;

  HRESULT hr = D3DCompileFromFile(
      fileName.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
      entryPoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

  if (errors != nullptr) {
    OutputDebugString((char *)errors->GetBufferPointer());
  }

  ThrowIfFailed(hr);
  return byteCode;
}

ComPtr<ID3D12Resource>
DXUtils::CreateDefaultBuffer(ID3D12Device *device,
                             ID3D12GraphicsCommandList *commandList,
                             const void *initData, UINT64 byteSize,
                             ComPtr<ID3D12Resource> &uploadBuffer) {
  ComPtr<ID3D12Resource> defaultBuffer;

  auto defaultBufferHeapProperties =
      CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  auto defaultBufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
  auto uploadBufferHeapProperties =
      CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  auto uploadBufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);

  ThrowIfFailed(device->CreateCommittedResource(
      &defaultBufferHeapProperties, D3D12_HEAP_FLAG_NONE,
      &defaultBufferResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
      IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

  ThrowIfFailed(device->CreateCommittedResource(
      &uploadBufferHeapProperties, D3D12_HEAP_FLAG_NONE,
      &uploadBufferResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
      IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

  auto subResourceData =
      D3D12_SUBRESOURCE_DATA{.pData = initData,
                             .RowPitch = static_cast<long>(byteSize),
                             .SlicePitch = static_cast<long>(byteSize)};

  auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
      defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON,
      D3D12_RESOURCE_STATE_COPY_DEST);
  commandList->ResourceBarrier(1, &barrier);

  UpdateSubresources<1>(commandList, defaultBuffer.Get(), uploadBuffer.Get(), 0,
                        0, 1, &subResourceData);

  auto barrierChanged = CD3DX12_RESOURCE_BARRIER::Transition(
      defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
      D3D12_RESOURCE_STATE_GENERIC_READ);
  commandList->ResourceBarrier(1, &barrierChanged);

  return defaultBuffer;
}

D3D12_VERTEX_BUFFER_VIEW MeshGeometry::VertexBufferView() const {
  return D3D12_VERTEX_BUFFER_VIEW{
      .BufferLocation = VertexGPUBuffer->GetGPUVirtualAddress(),
      .SizeInBytes = VertexBufferByteSize,
      .StrideInBytes = VertexByteStride,
  };
}

D3D12_INDEX_BUFFER_VIEW MeshGeometry::IndexBufferView() const {
  return {
      .BufferLocation = IndexGPUBuffer->GetGPUVirtualAddress(),
      .SizeInBytes = IndexBufferByteSize,
      .Format = IndexFormat,
  };
}

void MeshGeometry::DisposeUploaders() {
  VertexBufferUploader.Reset();
  IndexBufferUploader.Reset();
}
