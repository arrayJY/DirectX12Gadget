//
// Created by arrayJY on 2023/02/07.
//

#pragma once

#include "stdafx.h"

template <typename T> class UploadBuffer {
public:
  UploadBuffer(ID3D12Device *device, int elementCount);
  UploadBuffer(const UploadBuffer &) = delete;
  UploadBuffer &operator=(const UploadBuffer &) = delete;

  void Upload();
  ID3D12Resource *Resource(); 
  void CopyData(int elementIndex, const T &data);

private:
  ID3D12Device *device;
  int elementCount;
  size_t elementSize = sizeof(T);
  Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
  BYTE *mappedData;
};
