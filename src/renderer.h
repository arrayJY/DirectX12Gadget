//
// Created by arrayJY on 2023/02/02.
//

#pragma once

#include "stdafx.h"
#include "timer.h"

class Renderer {
public:
  struct InitInfo {
    unsigned width;
    unsigned height;
    HWND hwnd;
  };
  Renderer();
  static Renderer *GetRenderer();

  virtual void InitDirectX(const InitInfo &initInfo);

  virtual void OnResize(UINT width, UINT height);

  void FlushCommandQueue();

  void DrawFrame();

  virtual void KeyboardInput(int key, int scancode, int action, int mods);
  virtual void MousePostionInput(double xPos, double yPos);
  virtual void MouseButtonInput(int button, int action, int mods);

  static void OnResizeFrame(struct GLFWwindow *window, int width, int height);
  static void OnKeyboardInput(struct GLFWwindow *window, int key, int scancode,
                              int action, int mods);
  static void OnMousePostionInput(struct GLFWwindow *window, double xPos,
                                  double yPos);
  static void OnMouseButtonInput(struct GLFWwindow *window, int button,
                                 int action, int mods);

protected:
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
  void WaitForFence(UINT64 value);
  std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();

  ID3D12Resource *CurrentBackBuffer() const;
  D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
  D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;
  float AspectRatio() const;

  virtual void Draw(const GameTimer &timer) = 0;
  virtual void Update(const GameTimer &timer) = 0;

protected:
  static Renderer *renderer;
  static constexpr int swapChainBufferCount = 2;
  int currentBackBufferIndex = 0;

  ComPtr<IDXGIFactory4> dxgiFactory;
  ComPtr<ID3D12Device> device;

  ComPtr<ID3D12Fence> fence;
  UINT64 fenceValue;

  bool msaaState = false;
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
  DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

  UINT rtvDescriptorSize;
  UINT dsvDescriptorSize;
  UINT cbvUavDescriptorSize;

  GameTimer timer;

  UINT Width, Height;
};
