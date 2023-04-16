//
// Created by arrayJY on 2023/04/16.
//

#pragma once

#include "../renderer.h"
#include "../stdafx.h"
#include "frame_resource.h"

class ClothRenderer : public Renderer {
public:
  void InitDirectX(const InitInfo &initInfo) override;
  void OnResize(UINT width, UINT height) override;
  void Update(const GameTimer &timer) override;
  void Draw(const GameTimer &timer) override;

protected:
  void CreateBuffers();
  void CreateRootSignature();
  void CreateShadersAndInputLayout();
  void CreatePSOs();

  void DoCompute();

  ComPtr<ID3D12RootSignature> RootSignature = nullptr;
  ComPtr<ID3D12DescriptorHeap> SrvDescriptorHeap = nullptr;
  ComPtr<ID3D12Resource> VertexInputUploadBuffer = nullptr;
  ComPtr<ID3D12Resource> VertexInputBuffer = nullptr;
  ComPtr<ID3D12Resource> VertexOutputBuffer = nullptr;

  std::vector<Vertex> Vertecies;

  std::unordered_map<std::string, ComPtr<ID3DBlob>> Shaders;
  std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> PSOs;
};
