//
// Created by arrayJY on 2023/04/16.
//

#pragma once

#include "../renderer.h"
#include "../stdafx.h"
#include "cloth.h"
#include "frame_resource.h"
#include "render_item.h"


class ClothRenderer : public Renderer
{
public:
  void InitDirectX(const InitInfo& initInfo) override;
  void OnResize(UINT width, UINT height) override;
  void Update(const GameTimer& timer) override;
  void Draw(const GameTimer& timer) override;

protected:
  void LoadCloth();
  void CreateRootSignature();
  void CreateClothRootSignature();
  void CreateDescriptorHeaps();
  void CreateRenderItems();
  void CreateFrameResources();
  void CreateShadersAndInputLayout();
  void CreatePSOs();

  std::vector<Vertex> Vertecies;
  std::unique_ptr<Cloth> ClothSimulator;

  std::vector<std::unique_ptr<FrameResource>> FrameResources;
  FrameResource* CurrentFrameResource = nullptr;
  std::vector<std::unique_ptr<RenderItem>> AllRenderItems;

  ComPtr<ID3D12RootSignature> RootSignature = nullptr;
  ComPtr<ID3D12RootSignature> ClothRootSignature = nullptr;

  ComPtr<ID3D12DescriptorHeap> SrvUavCbvHeap = nullptr;

  std::unordered_map<std::string, ComPtr<ID3DBlob>> Shaders;
  std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayouts;

  std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> PSOs;

};
