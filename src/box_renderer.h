//
// Created by arrayJY on 2023/02/08.
//

#pragma once
#include "stdafx.h"
#include "renderer.h"

struct ConstantObject {};

class BoxRenderer : public Renderer {
public:
  void CreateConstantBuffer();
  void CreateRootSignature();
  void CreateShadersAndInputLayout();
  void CreatePSO();

private:
  ComPtr<ID3D12DescriptorHeap> cbvHeap = nullptr;
  ComPtr<ID3D12RootSignature> rootSignature = nullptr;
  std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
  ComPtr<ID3DBlob> vertexShader = nullptr;
  ComPtr<ID3DBlob> pixelShader = nullptr;
  ComPtr<ID3D12PipelineState> PSO = nullptr;
};