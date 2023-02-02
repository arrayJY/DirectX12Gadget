//
// Created by arrayJY on 2023/02/02.
//

#include "renderer.h"

bool Renderer::InitDirectX(const InitInfo &initInfo) {
  HRESULT hr;

  // Create device
  IDXGIFactory4 *dxgiFactory;
  hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
  if (FAILED(hr)) {
    return false;
  }

  IDXGIAdapter1 *adapter;
  auto adapterIndex = 0;
  bool adapterFound = false;

  while (dxgiFactory->EnumAdapters1(adapterIndex, &adapter) !=
         DXGI_ERROR_NOT_FOUND) {
    DXGI_ADAPTER_DESC1 desc;
    adapter->GetDesc1(&desc);

    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
      adapterIndex++;
      continue;
    }

    HRESULT result = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0,
                                       __uuidof(ID3D12Device), nullptr);
    if (SUCCEEDED(result)) {
      adapterFound = true;
      break;
    }
  }

  if (!adapterFound) {
    return false;
  }

  hr =
      D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));

  if (FAILED(hr)) {
    return false;
  }

  // Create command queue
  D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
  hr = device->CreateCommandQueue(&commandQueueDesc,
                                  IID_PPV_ARGS(&commandQueue));

  if (FAILED(hr)) {
    return false;
  }

  // Create swap chain
  DXGI_MODE_DESC backBufferDesc{.Width = initInfo.width,
                                .Height = initInfo.height,
                                .Format = DXGI_FORMAT_R8G8B8A8_UNORM};

  DXGI_SAMPLE_DESC sampleDesc{.Count = 1};

  DXGI_SWAP_CHAIN_DESC swapChainDesc{
      .BufferDesc = backBufferDesc,
      .SampleDesc = sampleDesc,
      .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
      .BufferCount = frameBufferCount,
      .OutputWindow = initInfo.hwnd,
      .Windowed = !initInfo.fullScreen,
      .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
  };

  IDXGISwapChain* tempSwapChain;
  dxgiFactory->CreateSwapChain(commandQueue, &swapChainDesc, &tempSwapChain);
  swapChain = static_cast<IDXGISwapChain3*>(tempSwapChain);
  frameIndex = swapChain->GetCurrentBackBufferIndex();



  return true;
}

void Renderer::Update() {}

void Renderer::UpdatePipeline() {}

void Renderer::Render() {}

void Renderer::Cleanup() {}

void Renderer::WaitForPreviousFrame() {}
