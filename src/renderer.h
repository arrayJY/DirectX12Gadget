//
// Created by arrayJY on 2023/02/02.
//

#pragma once

#include "stdafx.h"

class Renderer {
public:
  struct InitInfo {
    unsigned width;
    unsigned height;
    HWND hwnd;
  };

  // function declarations
  void InitDirectX(const InitInfo &initInfo); // initializes direct3d 12

  void OnResize(UINT width, UINT height);

  void FlushCommandQueue();

private:
  void CreateDevice();
  void CreateFence();
  void CollectDescriptorSize();
  void CollectMsaaInfo();
  void CreateCommandObjects();
  void CreateSwapChain(HWND hwnd, UINT width, UINT height);
  void CreateDesciptorHeaps();
  void CreateRenderTargetView();
  void CreateDepthStencilBuffer(UINT width, UINT height);
  void CreateDepthStencilView();
  void CreateViewportAndScissorRect(UINT width, UINT height);

  D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

protected:
  static constexpr int swapChainBufferCount = 2;
  int currentBackBufferIndex = 0;

  ComPtr<IDXGIFactory4> dxgiFactory;
  ComPtr<ID3D12Device> device;

  ComPtr<ID3D12Fence> fence;
  int fenceValue;

  bool msaaState = true;
  UINT msaaQuality;

  ComPtr<ID3D12CommandQueue> commandQueue;
  ComPtr<ID3D12CommandAllocator> commandAllocator;
  ComPtr<ID3D12GraphicsCommandList> commandList;

  ComPtr<IDXGISwapChain> swapChain;

  ComPtr<ID3D12DescriptorHeap> rtvHeap;
  ComPtr<ID3D12DescriptorHeap> dsvHeap;

  ComPtr<ID3D12Resource> swapChainBuffer[swapChainBufferCount];
  ComPtr<ID3D12Resource> depthStencilBuffer;

  D3D12_VIEWPORT viewport;
  D3D12_RECT scissorRect;

  DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
  DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

  UINT rtvDescriptorSize;
  UINT dsvDescriptorSize;
  UINT cbvUavDescriptorSize;
};
