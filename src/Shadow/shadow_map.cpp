//
// Created by arrayJY on 2023/03/26.
//

#include "shadow_map.h"

ShadowMap::ShadowMap(ID3D12Device *device, UINT width, UINT height)
    : device(device), width(width), height(height) {
  viewport = D3D12_VIEWPORT{
      .TopLeftX = 0.0f,
      .TopLeftY = 0.0f,
      .Width = static_cast<float>(width),
      .Height = static_cast<float>(height),
      .MinDepth = 0.0f,
      .MaxDepth = 1.0f,
  };

  scissorRect = D3D12_RECT{.left = 0,
                           .top = 0,
                           .right = static_cast<int>(width),
                           .bottom = static_cast<int>(height)};

  BuildResource();
}

void ShadowMap::OnResize(UINT w, UINT h) {
  if (width == w && height == h) {
    return;
  }

  width = w;
  height = h;

  BuildResource();
  BuildDescriptors();
}

void ShadowMap::BuildDescriptors() {
  auto srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC{
      .Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
      .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
      .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
      .Texture2D = D3D12_TEX2D_SRV{
          .MostDetailedMip = 0,
          .MipLevels = 1,
          .PlaneSlice = 0,
          .ResourceMinLODClamp = 0.0f,
      }};
  device->CreateShaderResourceView(shadowMap.Get(), &srvDesc, CPUSrv);

  auto dsvDesc = D3D12_DEPTH_STENCIL_VIEW_DESC{
      .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
      .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
      .Flags = D3D12_DSV_FLAG_NONE,
      .Texture2D = D3D12_TEX2D_DSV{.MipSlice = 0}};
  device->CreateDepthStencilView(shadowMap.Get(), &dsvDesc, CPUDsv);
}
void ShadowMap::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSrv,
                                 CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSrv,
                                 CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDsv) {
  CPUSrv = cpuSrv;
  GPUSrv = gpuSrv;
  CPUDsv = cpuDsv;

  BuildDescriptors();
}

void ShadowMap::BuildResource() {
  auto texDesc = D3D12_RESOURCE_DESC{
      .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
      .Alignment = 0,
      .Width = width,
      .Height = height,
      .DepthOrArraySize = 1,
      .MipLevels = 1,
      .Format = format,
      .SampleDesc = DXGI_SAMPLE_DESC{.Count = 1, .Quality = 0},
      .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
      .Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
  };

  auto clearValue = D3D12_CLEAR_VALUE{
      .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
      .DepthStencil = D3D12_DEPTH_STENCIL_VALUE{.Depth = 1.0f, .Stencil = 0}};

  auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  ThrowIfFailed(device->CreateCommittedResource(
      &heapProperties, D3D12_HEAP_FLAG_NONE, &texDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, &clearValue,
      IID_PPV_ARGS(&shadowMap)));
}
