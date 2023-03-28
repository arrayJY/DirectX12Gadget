//
// Created by arrayJY on 2023/03/26.
//

#pragma once

#include "../camera.h"
#include "../renderer.h"
#include "frame_resource.h"
#include "shadow_map.h"


enum class RenderLayer : int {
  Opaque = 0,
  Count,
};

struct RenderItem {
  RenderItem() = default;
  RenderItem(const RenderItem &rhs) = delete;

  XMFLOAT4X4 World = MathHelper::Identity4x4();

  XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

  int NumFramesDirty = FrameResourceCount;

  UINT ObjCBIndex = -1;

  Material *Mat = nullptr;
  MeshGeometry *Geo = nullptr;

  D3D12_PRIMITIVE_TOPOLOGY PrimitiveType =
      D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

  UINT IndexCount = 0;
  UINT StartIndexLocation = 0;
  int BaseVertexLocation = 0;
};

class ShadowRenderer : public Renderer {
public:
  void InitDirectX(const InitInfo &initInfo) override;
  void OnResize(UINT width, UINT height) override;
  void Draw(const GameTimer &timer) override;
  void Update(const GameTimer &timer) override;

private:
  void LoadTextures();
  void CreateRootSignature();
  void CreateDescriptorHeaps();
  void CreateShadersAndInputLayout();
  void CreateShapeGeometry();
  void CreateMaterials();
  void CreateRenderItems();
  void CreateFrameResources();
  void CreatePSOs();

  void DrawRenderItems(
    ID3D12GraphicsCommandList *cmdList,
    const std::vector<RenderItem*> &renderItems);
  void DrawSceneToShadowMap();

  void UpdateObjectCBs(const GameTimer &timer);
  void UpdateMaterialBuffer(const GameTimer &timer);
  void UpdateShadowTransform(const GameTimer &timer);
  void UpdateMainPassCB(const GameTimer &timer);
  void UpdateShadowPassCB(const GameTimer &timer);

private:
  Camera camera;

  PassConstants MainPassCB;
  PassConstants ShadowPassCB;

  DirectX::XMFLOAT4X4 ShadowTransform = MathHelper::Identity4x4();
  DirectX::XMFLOAT3 LightDirections[3];

  DirectX::XMFLOAT4X4 LightView = MathHelper::Identity4x4();
  DirectX::XMFLOAT4X4 LightProj = MathHelper::Identity4x4();
  DirectX::XMFLOAT3 LightPosW = {0.0f, 0.0f, 0.0f};
  float LightNearZ = 1.0f;
  float LightFarZ = 1000.0f;

  ComPtr<ID3D12RootSignature> RootSignature = nullptr;
  ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap = nullptr;

  UINT ShadowMapHeapIndex = 0;
  UINT NullTextureSrvIndex = 0;
  CD3DX12_GPU_DESCRIPTOR_HANDLE NullSrv;

  std::vector<std::unique_ptr<FrameResource>> FrameResources;
  FrameResource* CurrentFrameResource = nullptr;
  int CurrentFrameResourceIndex = 0;

  std::vector<std::unique_ptr<RenderItem>> AllRenderItems;
  std::vector<RenderItem*> LayerItems[(int)RenderLayer::Count];

  std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
  std::unique_ptr<ShadowMap> shadowMap = nullptr;
  std::unordered_map<std::string, ComPtr<ID3DBlob>> Shaders;
  std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> PSOs;
  std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> Geometries;
  std::unordered_map<std::string, std::unique_ptr<Material>> Materials;
};
