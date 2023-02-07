//
// Created by arrayJY on 2023/02/07.
//

#include "upload_buffer.h"

template <typename T>
UploadBuffer<T>::UploadBuffer(ID3D12Device *device, int elementCount)
    : device(device), elementCount(elementCount) {}

template <typename T> void UploadBuffer<T>::Upload() {
  auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(elementSize * elementCount);
  ThrowIfFailed(device->CreateCommittedResource(
      &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer)));
  ThrowIfFailed(
      uploadBuffer->Map(0, nullptr, reinterpret_cast<void **>(&mappedData)));
}

template <typename T> ID3D12Resource *UploadBuffer<T>::Resource() {
  return uploadBuffer.Get();
}

template <typename T>
void UploadBuffer<T>::CopyData(int elementIndex, const T &data) {
  std::memcpy(&mappedData[elementIndex * elementSize], &data, sizeof(T));
}
