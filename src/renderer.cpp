//
// Created by arrayJY on 2023/02/02.
//

#include "renderer.h"
#include <array>
#include <combaseapi.h>
#include <d3d12.h>
#include <stdexcept>

void Renderer::InitDirectX(const InitInfo &initInfo) {
#if defined(DEBUG)
  {
    Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
    debugController->EnableDebugLayer();
  }
#endif

  CreateDevice();
  CreateFence();
  CollectDescriptorSize();
  CollectMsaaInfo();
  CreateCommandObjects();
  CreateSwapChain(initInfo.hwnd, initInfo.width, initInfo.height);
  CreateDesciptorHeaps();
  OnResize(initInfo.width, initInfo.height);
}

void Renderer::OnResize(UINT width, UINT height) {
  assert(device);
  assert(swapChain);
  assert(commandAllocator);

  FlushCommandQueue();
  commandList->Reset(commandAllocator.Get(), nullptr);

  for (auto i = 0; i < swapChainBufferCount; i++) {
    swapChainBuffer[i].Reset();
  }

  depthStencilBuffer.Reset();
  swapChain->ResizeBuffers(swapChainBufferCount, width, height,
                           backBufferFormat,
                           DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
  currentBackBufferIndex = 0;
  CreateRenderTargetView();
  CreateDepthStencilBuffer(width, height);
  CreateDepthStencilView();
  CreateViewportAndScissorRect(width, height);
}

void Renderer::FlushCommandQueue() {
  fenceValue++;
  ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValue));

  if (fence->GetCompletedValue() < fenceValue) {
    HANDLE eventHandle =
        CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

    ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, eventHandle));

    WaitForSingleObject(eventHandle, INFINITE);
    CloseHandle(eventHandle);
  }
}

void Renderer::CreateDevice() {
  ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));

  auto hardwareResult =
      D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
  if (FAILED(hardwareResult)) {
    ComPtr<IDXGIAdapter> wrapAdapter;
    ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&wrapAdapter)));
    ThrowIfFailed(D3D12CreateDevice(wrapAdapter.Get(), D3D_FEATURE_LEVEL_11_0,
                                    IID_PPV_ARGS(&device)));
  }
}

void Renderer::CreateFence() {
  ThrowIfFailed(
      device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
}

void Renderer::CollectDescriptorSize() {
  rtvDescriptorSize =
      device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  dsvDescriptorSize =
      device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
  cbvUavDescriptorSize = device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void Renderer::CollectMsaaInfo() {
  auto msQualityLevels = D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS{
      .Format = backBufferFormat,
      .SampleCount = 4,
      .Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE,
      .NumQualityLevels = 0,
  };

  ThrowIfFailed(
      device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
                                  &msQualityLevels, sizeof(msQualityLevels)));
  msaaQuality = msQualityLevels.NumQualityLevels;
  assert(msaaQuality > 0 && "Unexpected MSAA quality level.");
}

void Renderer::CreateCommandObjects() {
  auto queueDesc = D3D12_COMMAND_QUEUE_DESC{
      .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
      .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
  };

  ThrowIfFailed(
      device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

  ThrowIfFailed(device->CreateCommandAllocator(
      D3D12_COMMAND_LIST_TYPE_DIRECT,
      IID_PPV_ARGS(commandAllocator.GetAddressOf())));

  ThrowIfFailed(device->CreateCommandList(
      0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr,
      IID_PPV_ARGS(commandList.GetAddressOf())));

  commandList->Close();
}

void Renderer::CreateSwapChain(HWND hwnd, UINT width, UINT height) {
  swapChain.Reset();
  auto swapChainDesc = DXGI_SWAP_CHAIN_DESC{
      .BufferDesc =
          DXGI_MODE_DESC{
              .Width = width,
              .Height = height,
              .RefreshRate =
                  DXGI_RATIONAL{
                      .Numerator = 60,
                      .Denominator = 1,
                  },
              .Format = backBufferFormat,
              .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
              .Scaling = DXGI_MODE_SCALING_UNSPECIFIED,
          },
      .SampleDesc =
          DXGI_SAMPLE_DESC{.Count = msaaState ? 4U : 1U,
                           .Quality = msaaState ? (msaaQuality - 1) : 0},
      .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
      .BufferCount = swapChainBufferCount,
      .OutputWindow = hwnd,
      .Windowed = true,
      .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
      .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH,
  };

  ThrowIfFailed(dxgiFactory->CreateSwapChain(commandQueue.Get(), &swapChainDesc,
                                             swapChain.GetAddressOf()));
}

void Renderer::CreateDesciptorHeaps() {
  auto rtvHeapDesc = D3D12_DESCRIPTOR_HEAP_DESC{
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
      .NumDescriptors = swapChainBufferCount,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
  };

  auto dsvHeapDesc = D3D12_DESCRIPTOR_HEAP_DESC{
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
      .NumDescriptors = 1,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
      .NodeMask = 0,
  };

  ThrowIfFailed(device->CreateDescriptorHeap(
      &rtvHeapDesc, IID_PPV_ARGS(rtvHeap.GetAddressOf())));
  ThrowIfFailed(device->CreateDescriptorHeap(
      &dsvHeapDesc, IID_PPV_ARGS(dsvHeap.GetAddressOf())));
}

void Renderer::CreateRenderTargetView() {
  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(
      rtvHeap->GetCPUDescriptorHandleForHeapStart());
  for (auto i = 0; i < swapChainBufferCount; i++) {
    ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&swapChainBuffer[i])));
    device->CreateRenderTargetView(swapChainBuffer[i].Get(), nullptr,
                                   rtvHeapHandle);
    rtvHeapHandle.Offset(1, rtvDescriptorSize);
  }
}

void Renderer::CreateDepthStencilBuffer(UINT width, UINT height) {
  auto depthStencilDesc =
      D3D12_RESOURCE_DESC{.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                          .Alignment = 0,
                          .Width = width,
                          .Height = height,
                          .DepthOrArraySize = 1,
                          .MipLevels = 1,
                          .Format = depthStencilFormat,
                          .SampleDesc =
                              DXGI_SAMPLE_DESC{
                                  .Count = msaaState ? 4U : 1U,
                                  .Quality = msaaState ? (msaaQuality - 1) : 0,
                              },
                          .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
                          .Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL};

  auto clearValue = D3D12_CLEAR_VALUE{
      .Format = depthStencilFormat,
      .DepthStencil = D3D12_DEPTH_STENCIL_VALUE{.Depth = 1.0f, .Stencil = 0},
  };

  auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

  ThrowIfFailed(device->CreateCommittedResource(
      &heapProperties, D3D12_HEAP_FLAG_NONE, &depthStencilDesc,
      D3D12_RESOURCE_STATE_COMMON, &clearValue,
      IID_PPV_ARGS(depthStencilBuffer.GetAddressOf())));
}

void Renderer::CreateDepthStencilView() {

  device->CreateDepthStencilView(depthStencilBuffer.Get(), nullptr,
                                 DepthStencilView());
  auto resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
      depthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON,
      D3D12_RESOURCE_STATE_DEPTH_WRITE);
  commandList->ResourceBarrier(1, &resourceBarrier);
}

void Renderer::CreateViewportAndScissorRect(UINT width, UINT height) {
  viewport = D3D12_VIEWPORT{
      .TopLeftX = 0,
      .TopLeftY = 0,
      .Width = static_cast<float>(width),
      .Height = static_cast<float>(height),
      .MinDepth = 0.0f,
      .MaxDepth = 1.0f,
  };

  scissorRect = D3D12_RECT{
      .left = 0,
      .top = 0,
      .right = static_cast<long>(width),
      .bottom = static_cast<long>(height),
  };
}

D3D12_CPU_DESCRIPTOR_HANDLE Renderer::DepthStencilView() const {
  return dsvHeap->GetCPUDescriptorHandleForHeapStart();
}
