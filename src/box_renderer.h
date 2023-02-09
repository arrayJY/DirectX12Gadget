//
// Created by arrayJY on 2023/02/08.
//

#pragma once
#include "dx_utils.h"
#include "math_helper.h"
#include "renderer.h"
#include "stdafx.h"
#include "upload_buffer.h"

struct Vertex {
  DirectX::XMFLOAT3 Pos;
  DirectX::XMFLOAT4 Color;
};

struct ConstantObject {
  DirectX::XMFLOAT4X4 MVP = MathHelper::Identity4x4();
};

class BoxRenderer : public Renderer {
public:
  void InitDirectX(const InitInfo &initInfo) override;

  void BuildDescriptorHeaps();
  void CreateConstantBuffer();
  void CreateRootSignature();
  void CreateShadersAndInputLayout();
  void CreateBoxGeometry();
  void CreatePSO();

  void Draw(const GameTimer &timer) override;
  void Update(const GameTimer &timer) override;

private:
  ComPtr<ID3D12DescriptorHeap> cbvHeap = nullptr;
  ComPtr<ID3D12RootSignature> rootSignature = nullptr;
  std::unique_ptr<UploadBuffer<ConstantObject>> uploadBuffer = nullptr;
  std::unique_ptr<MeshGeometry> BoxGeometry = nullptr;

  std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
  ComPtr<ID3DBlob> vertexShader = nullptr;
  ComPtr<ID3DBlob> pixelShader = nullptr;
  ComPtr<ID3D12PipelineState> PSO = nullptr;

  XMFLOAT4X4 worldMatrix = MathHelper::Identity4x4();
  XMFLOAT4X4 viewMatrix = MathHelper::Identity4x4();
  XMFLOAT4X4 projectionMatrix = MathHelper::Identity4x4();
  float theta = 1.5f * DirectX::XM_PI;
  float phi = DirectX::XM_PIDIV4;
  float radius = 5.0f;
};