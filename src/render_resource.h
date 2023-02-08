//
// Created by arrayJY on 2023/02/08.
//

#pragma once
#include "stdafx.h"

struct ConstantObject {};

class Box {
public:
  Box() = delete;
  Box(ID3D12Device *device);
  void CreateConstantBuffer();
  void CreateRootSignature();

private:
  ID3D12Device *device = nullptr;
  ComPtr<ID3D12DescriptorHeap> cbvHeap = nullptr;
  ComPtr<ID3D12RootSignature> rootSignature = nullptr;
};