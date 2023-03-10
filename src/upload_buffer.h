//
// Created by arrayJY on 2023/02/07.
//

#pragma once

#include "dx_utils.h"
#include "stdafx.h"

template <typename T> class UploadBuffer {
public:
  UploadBuffer(ID3D12Device *device, int elementCount);
  ~UploadBuffer();
  UploadBuffer(const UploadBuffer &) = delete;
  UploadBuffer &operator=(const UploadBuffer &) = delete;

  ID3D12Resource *Resource();
  void CopyData(int elementIndex, const T &data);

private:
  ID3D12Device *device;
  int elementCount;
  size_t elementSize = sizeof(T);
  Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
  BYTE *mappedData;
};

template <typename T>
inline UploadBuffer<T>::UploadBuffer(ID3D12Device *device, int elementCount)
    : device(device), elementCount(elementCount) {
  elementSize = DXUtils::CalcConstantBufferSize(sizeof(T));
  auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(elementSize * elementCount);
  ThrowIfFailed(device->CreateCommittedResource(
      &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer)));
  ThrowIfFailed(
      uploadBuffer->Map(0, nullptr, reinterpret_cast<void **>(&mappedData)));
}

template <typename T> inline UploadBuffer<T>::~UploadBuffer() {
  if (uploadBuffer != nullptr) {
    uploadBuffer->Unmap(0, nullptr);
  }
  mappedData = nullptr;
}

template <typename T> inline ID3D12Resource *UploadBuffer<T>::Resource() {
  return uploadBuffer.Get();
}

template <typename T>
inline void UploadBuffer<T>::CopyData(int elementIndex, const T &data) {
  std::memcpy(&mappedData[elementIndex * elementSize], &data, sizeof(T));
}