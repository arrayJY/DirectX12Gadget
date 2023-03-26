//
// Created by arrayJY on 2023/03/26.
//

#pragma once

#include "../renderer.h"
#include "shadow_map.h"

class ShadowRenderer : public Renderer {
public:
  void InitDirectX(const InitInfo &initInfo) override;
  void OnResize(UINT width, UINT height) override;

private:
  void LoadTextures();
  void CreateRootSignature();
  void CreateDescriptorHeaps();
  void CreateShadersAndInputLayout();
  void CreateShapeGeometry();
  void CreatePSOs();

private:
  ComPtr<ID3D12RootSignature> RootSignature = nullptr;
  ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap = nullptr;

  UINT ShadowMapHeapIndex = 0;
  UINT NullTextureSrvIndex = 0;
  CD3DX12_GPU_DESCRIPTOR_HANDLE NullSrv;

  std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
  std::unique_ptr<ShadowMap> shadowMap = nullptr;
  std::unordered_map<std::string, ComPtr<ID3DBlob>> Shaders;
  std::unordered_map<std::string, ComPtr<ID3D12Resource>> PSOs;
};
