//
// Created by arrayJY on 2023/03/26.
//

#include "shadow_renderer.h"

void ShadowRenderer::InitDirectX(const InitInfo &initInfo) {
  Renderer::InitDirectX(initInfo);

  ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));

  shadowMap = std::make_unique<ShadowMap>(device.Get(), 2048, 2048);

  LoadTextures();
  CreateRootSignature();
  CreateDescriptorHeaps();
  CreateShadersAndInputLayout();
  CreateShapeGeometry();
  CreatePSOs();
}
void ShadowRenderer::OnResize(UINT width, UINT height) {}

void ShadowRenderer::LoadTextures() {
  // TODO
}

void ShadowRenderer::CreateRootSignature() {
  CD3DX12_DESCRIPTOR_RANGE texTable[2];
  texTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0);
  texTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 2);

  CD3DX12_ROOT_PARAMETER slotRootParameter[5];
  slotRootParameter[0].InitAsConstantBufferView(0);
  slotRootParameter[1].InitAsConstantBufferView(1);
  slotRootParameter[2].InitAsShaderResourceView(0, 1);
  slotRootParameter[3].InitAsDescriptorTable(1, &texTable[0],
                                             D3D12_SHADER_VISIBILITY_PIXEL);
  slotRootParameter[4].InitAsDescriptorTable(1, &texTable[1],
                                             D3D12_SHADER_VISIBILITY_PIXEL);

  auto staticSamplers = GetStaticSamplers();

  CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
      5, slotRootParameter, static_cast<UINT>(staticSamplers.size()),
      staticSamplers.data(),
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
  ComPtr<ID3DBlob> serializedRootSig = nullptr;
  ComPtr<ID3DBlob> errorBlob = nullptr;
  HRESULT hr = D3D12SerializeRootSignature(
      &rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
      serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

  if (errorBlob != nullptr) {
    ::OutputDebugStringA((char *)errorBlob->GetBufferPointer());
  }
  ThrowIfFailed(hr);

  ThrowIfFailed(
      device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(),
                                  serializedRootSig->GetBufferSize(),
                                  IID_PPV_ARGS(RootSignature.GetAddressOf())));
}

void ShadowRenderer::CreateDescriptorHeaps() {
  // SRV
  auto srvHeapDesc = D3D12_DESCRIPTOR_HEAP_DESC{
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = 14,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
  };
  ThrowIfFailed(device->CreateDescriptorHeap(
      &srvHeapDesc, IID_PPV_ARGS(srvDescriptorHeap.GetAddressOf())));

  CD3DX12_CPU_DESCRIPTOR_HANDLE descriptorHandle(
      srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
  std::vector<ComPtr<ID3D12Resource>> texturesList; // TODO: fill texture list

  auto srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC{
      .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
      .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
      .Texture2D =
          D3D12_TEX2D_SRV{
              .MostDetailedMip = 0,
              .ResourceMinLODClamp = 0.0f,
          },
  };

  for (auto &i : texturesList) {
    srvDesc.Format = i->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = i->GetDesc().MipLevels;
    device->CreateShaderResourceView(i.Get(), &srvDesc, descriptorHandle);
    descriptorHandle.Offset(1, cbvUavDescriptorSize);
  }

  ShadowMapHeapIndex = texturesList.size();
  NullTextureSrvIndex = ShadowMapHeapIndex + 1;

  auto srvCPUStart = srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
  auto srvGPUStart = srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
  auto dsvCPUStart = dsvHeap->GetCPUDescriptorHandleForHeapStart();

  auto nullSrv = CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCPUStart, NullTextureSrvIndex,
                                               cbvUavDescriptorSize);
  NullSrv = CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGPUStart, NullTextureSrvIndex,
                                          cbvUavDescriptorSize);

  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  srvDesc.Texture2D = D3D12_TEX2D_SRV{
      .MostDetailedMip = 0,
      .MipLevels = 1,
      .PlaneSlice = 0,
      .ResourceMinLODClamp = 0.0f,
  };

  device->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);

  shadowMap->BuildDescriptors(
      CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCPUStart, ShadowMapHeapIndex,
                                    cbvUavDescriptorSize),
      CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGPUStart, ShadowMapHeapIndex,
                                    cbvUavDescriptorSize),
      CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvCPUStart, 1, dsvDescriptorSize));
}

void ShadowRenderer::CreateShadersAndInputLayout() {
  constexpr D3D_SHADER_MACRO alphaTestDefines[] = {"ALPHA_TEST", "1", nullptr,
                                                   nullptr};

  Shaders["shadowVS"] = DXUtils::CompileShader(SHADER_DIR L"/shadow.hlsl",
                                               nullptr, "VS", "vs_5_1");
  Shaders["shadowOpaquePS"] = DXUtils::CompileShader(SHADER_DIR L"/shadow.hlsl",
                                                     nullptr, "PS", "ps_5_1");
  Shaders["shadowAlphaPS"] = DXUtils::CompileShader(SHADER_DIR L"/shadow.hlsl",
                                                    nullptr, "PS", "ps_5_1");

  inputLayout = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  };
}

void ShadowRenderer::CreateShapeGeometry() {}

void ShadowRenderer::CreatePSOs() {
  // Shadow map pass
  auto smaplePsoDesc = D3D12_GRAPHICS_PIPELINE_STATE_DESC{
      .pRootSignature = RootSignature.Get(),
      .VS = CD3DX12_SHADER_BYTECODE(Shaders["shadowVS"].Get()),
      .PS = CD3DX12_SHADER_BYTECODE(Shaders["shadowOpaquePS"].Get()),
      .RasterizerState =
          {
              .DepthBias = 100000,
              .DepthBiasClamp = 1.0f,
              .SlopeScaledDepthBias = 1.0f,
          },
      .NumRenderTargets = 0,
      .RTVFormats = {DXGI_FORMAT_UNKNOWN},
  };
  ThrowIfFailed(device->CreateGraphicsPipelineState(
      &smaplePsoDesc, IID_PPV_ARGS(&PSOs["shadowOpaque"])));
}
