//
// Created by arrayJY on 2023/02/14.
//

#pragma once

#include "../renderer.h"
#include "../stdafx.h"
#include "frame_resource.h"
#include "render_item.h"

struct RenderItem;

class PBRRenderer : public Renderer {
public:
  PBRRenderer() : Renderer() { renderer = this; }

  void InitDirectX(const InitInfo &initInfo) override;
  void Update(const GameTimer &timer) override;
  void Draw(const GameTimer &timer) override;
  void DrawRenderItems(ID3D12GraphicsCommandList *cmdList,
                       std::vector<RenderItem *> items);
  void OnResize(UINT width, UINT height) override;

  static int GetFrameResourceCount() { return FrameResourceCount; }

  void KeyboardInput(int key, int scancode, int action, int mods) override;
  void MouseButtonInput(int button, int action, int mods) override;
  void MousePostionInput(double xPos, double yPos) override;

private:
  void CreateRootSignature();
  void CreateShaderAndInputLayout();
  void CreateShapeGeometry();
  void CreateMaterials();
  void CreateRenderItems();
  void CreateFrameResource();
  void CreateDescriptorHeaps();
  void CreateConstantBufferView();
  void CreatePSOs();

  void UpdateCamera(const GameTimer &timer);
  void UpdateObjectConstantsBuffer(const GameTimer &timer);
  void UpdateMaterialConstantsBuffer(const GameTimer &timer);
  void UpdateMainPassConstantsBuffer(const GameTimer &timer);

private:
  static const int FrameResourceCount = 3;
  std::vector<std::unique_ptr<FrameResource>> FrameResources;
  FrameResource *CurrentFrameResource;
  int CurrentFrameResourceIndex = 0;
  bool IsWireFrame = false;
  bool MousePressed = false;
  double LastMousePosX = 0.0, LastMousePosY = 0.0;

  std::unordered_map<std::string, std::unique_ptr<PBRMaterial>> Materials;
  std::vector<std::unique_ptr<RenderItem>> AllRenderItems;
  std::vector<RenderItem *> OpaqueRenderItems;

  PassConstants MainPassConstants;
  UINT PassCbvOffset = 0U;

  XMFLOAT4X4 ModelMatrix = MathHelper::Identity4x4();
  XMFLOAT4X4 ViewMatrix = MathHelper::Identity4x4();
  XMFLOAT4X4 ProjectionMatrix = MathHelper::Identity4x4();

  XMFLOAT3 EyePos = {0.0f, 0.0f, 0.0f};

  float Theta = 1.5f * DirectX::XM_PI;
  float Phi = 0.2f * DirectX::XM_PI;
  float Radius = 15.0f;

  ComPtr<ID3D12RootSignature> RootSignature = nullptr;
  std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayout;

  ComPtr<ID3D12DescriptorHeap> cbvHeap = nullptr;

  std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> Geometries;
  std::unordered_map<std::string, ComPtr<ID3DBlob>> Shaders;
  std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> PSOs;
};