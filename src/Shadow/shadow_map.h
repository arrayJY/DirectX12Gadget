//
// Created by arrayJY on 2023/03/26.
//

#pragma once

#include "../dx_utils.h"

class ShadowMap {
public:
  ShadowMap(ID3D12Device *device, UINT width, UINT height);
  ShadowMap(const ShadowMap &rhs) = delete;
  ShadowMap &operator=(const ShadowMap &rhs) = delete;

  void OnResize(UINT width, UINT height);

  UINT Width() const { return width; }
  UINT Height() const { return height; }
  CD3DX12_GPU_DESCRIPTOR_HANDLE Srv() const { return GPUSrv; }
  CD3DX12_CPU_DESCRIPTOR_HANDLE Dsv() const { return CPUDsv; }

  D3D12_VIEWPORT Viewport() const { return viewport; }
  D3D12_RECT ScissorRect() const { return scissorRect; }

  void BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSrv,
                        CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSrv,
                        CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDsv);

private:
  void BuildDescriptors();
  void BuildResource();

private:
  ID3D12Device *device = nullptr;

  D3D12_VIEWPORT viewport;
  D3D12_RECT scissorRect;

  UINT width = 0;
  UINT height = 0;
  DXGI_FORMAT format = DXGI_FORMAT_R24G8_TYPELESS;

  CD3DX12_CPU_DESCRIPTOR_HANDLE CPUSrv;
  CD3DX12_GPU_DESCRIPTOR_HANDLE GPUSrv;
  CD3DX12_CPU_DESCRIPTOR_HANDLE CPUDsv;

  ComPtr<ID3D12Resource> shadowMap;
};
