//
// Created by arrayJY on 2023/02/02.
//

#include "renderer.h"
#include <array>
#include <combaseapi.h>
#include <d3d12.h>
#include <stdexcept>

Renderer *Renderer::renderer = nullptr;
Renderer::Renderer() { renderer = this; }

Renderer *Renderer::GetRenderer() { return renderer; }

void Renderer::OnResizeFrame(struct GLFWwindow *window, int width, int height) {
  GetRenderer()->OnResize(width, height);
}

void Renderer::OnKeyboardInput(struct GLFWwindow *window, int key, int scancode,
                               int action, int mods) {

  GetRenderer()->KeyboardInput(key, scancode, action, mods);
}

void Renderer::OnMousePostionInput(struct GLFWwindow *window, double xPos,
                                   double yPos) {
  GetRenderer()->MousePostionInput(xPos, yPos);
}

void Renderer::OnMouseButtonInput(struct GLFWwindow *window, int button,
                                  int action, int mods) {
  GetRenderer()->MouseButtonInput(button, action, mods);
}

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
  Width = width, Height = height;

  FlushCommandQueue();
  ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));

  for (auto i = 0; i < swapChainBufferCount; i++) {
    swapChainBuffer[i].Reset();
  }

  depthStencilBuffer.Reset();
  ThrowIfFailed(swapChain->ResizeBuffers(
      swapChainBufferCount, width, height, backBufferFormat,
      DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
  currentBackBufferIndex = 0;
  CreateRenderTargetView();
  CreateDepthStencilBuffer(width, height);
  CreateDepthStencilView();

  ThrowIfFailed(commandList->Close());
  ID3D12CommandList *cmdsLists[] = {commandList.Get()};
  commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
  FlushCommandQueue();

  CreateViewportAndScissorRect(width, height);
}

void Renderer::FlushCommandQueue() {
  fenceValue++;
  ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValue));

  if (fence->GetCompletedValue() < fenceValue) {
    WaitForFence(fenceValue);
  }
}

void Renderer::DrawFrame() {
  timer.Tick();
  Update(timer);
  Draw(timer);
}

void Renderer::KeyboardInput(int key, int scancode, int action, int mods) {}

void Renderer::MousePostionInput(double xPos, double yPos) {}

void Renderer::MouseButtonInput(int button, int action, int mods) {}

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

  ThrowIfFailed(
      device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));
  ThrowIfFailed(
      device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap)));
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
                          .Format = DXGI_FORMAT_R24G8_TYPELESS,
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
  auto dsvDesc = D3D12_DEPTH_STENCIL_VIEW_DESC{
      .Format = depthStencilFormat,
      .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
      .Flags = D3D12_DSV_FLAG_NONE,
      .Texture2D = D3D12_TEX2D_DSV{.MipSlice = 0},
  };

  device->CreateDepthStencilView(depthStencilBuffer.Get(), &dsvDesc,
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

void Renderer::WaitForFence(UINT64 value) {
  auto eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
  ThrowIfFailed(fence->SetEventOnCompletion(value, eventHandle));
  WaitForSingleObject(eventHandle, INFINITE);
  CloseHandle(eventHandle);
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> Renderer::GetStaticSamplers() {
  const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
      0,                                // shaderRegister
      D3D12_FILTER_MIN_MAG_MIP_POINT,   // filter
      D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
      D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
      D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW
  const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
      1,                                 // shaderRegister
      D3D12_FILTER_MIN_MAG_MIP_POINT,    // filter
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW
  const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
      2,                                // shaderRegister
      D3D12_FILTER_MIN_MAG_MIP_LINEAR,  // filter
      D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
      D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
      D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW
  const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
      3,                                 // shaderRegister
      D3D12_FILTER_MIN_MAG_MIP_LINEAR,   // filter
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW
  const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
      4,                               // shaderRegister
      D3D12_FILTER_ANISOTROPIC,        // filter
      D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressU
      D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressV
      D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressW
      0.0f,                            // mipLODBias
      8);                              // maxAnisotropy
  const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
      5,                                // shaderRegister
      D3D12_FILTER_ANISOTROPIC,         // filter
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressU
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressV
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressW
      0.0f,                             // mipLODBias
      8);                               // maxAnisotropy
  const CD3DX12_STATIC_SAMPLER_DESC shadow(
      6,                                                // shaderRegister
      D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,                 // addressU
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,                 // addressV
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,                 // addressW
      0.0f,                                             // mipLODBias
      16,                                               // maxAnisotropy
      D3D12_COMPARISON_FUNC_LESS_EQUAL, D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

  return {pointWrap,       pointClamp,       linearWrap, linearClamp,
          anisotropicWrap, anisotropicClamp, shadow};
}

ID3D12Resource *Renderer::CurrentBackBuffer() const {
  return swapChainBuffer[currentBackBufferIndex].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE Renderer::CurrentBackBufferView() const {
  return CD3DX12_CPU_DESCRIPTOR_HANDLE(
      rtvHeap->GetCPUDescriptorHandleForHeapStart(), currentBackBufferIndex,
      rtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE Renderer::DepthStencilView() const {
  return dsvHeap->GetCPUDescriptorHandleForHeapStart();
}

float Renderer::AspectRatio() const { return (float)Width / Height; }
