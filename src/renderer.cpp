//
// Created by arrayJY on 2023/02/02.
//

#include "renderer.h"
#include <array>
#include <combaseapi.h>
#include <d3d12.h>

bool Renderer::InitDirectX(const InitInfo &initInfo) {

#define RETURN_IF_FAILED(hr)                                                   \
  if (FAILED(hr)) {                                                            \
    return false;                                                              \
  }
#define STOP_RUNNING_IF_FAILED(hr)                                             \
  if (FAILED(hr)) {                                                            \
    Running = false;                                                           \
  }

#ifdef DEBUG
  {
    Microsoft::WRL::ComPtr<ID3D12Debug> debug;
    D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
    debug->EnableDebugLayer();
    debug = nullptr;
  }
#endif

  HRESULT hr;

  /* Create device */
  IDXGIFactory4 *dxgiFactory;
  hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
  RETURN_IF_FAILED(hr);

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
  RETURN_IF_FAILED(hr);

  /* Create command queue */
  D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
  hr = device->CreateCommandQueue(&commandQueueDesc,
                                  IID_PPV_ARGS(&commandQueue));

  RETURN_IF_FAILED(hr);

  /* Create swap chain */
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

  IDXGISwapChain *tempSwapChain;
  dxgiFactory->CreateSwapChain(commandQueue, &swapChainDesc, &tempSwapChain);
  swapChain = static_cast<IDXGISwapChain3 *>(tempSwapChain);
  frameIndex = swapChain->GetCurrentBackBufferIndex();

  /* Create render target descriptor heap */
  D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc{
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
      .NumDescriptors = frameBufferCount,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
  };

  hr = device->CreateDescriptorHeap(&rtvDescriptorHeapDesc,
                                    IID_PPV_ARGS(&rtvDescriptorHeap));
  RETURN_IF_FAILED(hr);

  rtvDescriptorSize =
      device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  CD3DX12_CPU_DESCRIPTOR_HANDLE
  rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
  for (auto i = 0; i < frameBufferCount; i++) {
    hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
    RETURN_IF_FAILED(hr);
    device->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);
    rtvHandle.Offset(1, rtvDescriptorSize);
  }

  /* Create the command allocators */
  for (auto i = 0; i < frameBufferCount; i++) {
    hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                        IID_PPV_ARGS(&commandAllocator[i]));
    RETURN_IF_FAILED(hr);
  }

  /* Create command list */
  hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                 commandAllocator[0], nullptr,
                                 IID_PPV_ARGS(&commandList));
  RETURN_IF_FAILED(hr);

  /* Create a fence & fence event */
  for (auto i = 0; i < frameBufferCount; i++) {
    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence[i]));
    RETURN_IF_FAILED(hr);
    fenceValue[i] = 0;
  }
  fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (!fenceEvent) {
    return false;
  }

  /* Create root signature */
  CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
  rootSignatureDesc.Init(
      0, nullptr, 0, nullptr,
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
  ID3DBlob *signature;
  hr = D3D12SerializeRootSignature(
      &rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
  RETURN_IF_FAILED(hr);
  hr = device->CreateRootSignature(0, signature->GetBufferPointer(),
                                   signature->GetBufferSize(),
                                   IID_PPV_ARGS(&rootSignature));
  RETURN_IF_FAILED(hr);

  /* Create vertex and pixel shaders */
  ID3DBlob *errorBuff;

  ID3DBlob *vertexShader;
  hr = D3DCompileFromFile(SHADER_DIR L"/VertexShader.hlsl", nullptr, nullptr,
                          "main", "vs_5_0",
                          D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
                          &vertexShader, &errorBuff);
  if (FAILED(hr)) {
    OutputDebugStringA((char *)errorBuff->GetBufferPointer());
    return false;
  }
  D3D12_SHADER_BYTECODE vertexShaderBytecode = {
      .pShaderBytecode = vertexShader->GetBufferPointer(),
      .BytecodeLength = vertexShader->GetBufferSize(),
  };

  ID3DBlob *pixelShader;
  hr = D3DCompileFromFile(SHADER_DIR L"/PixelShader.hlsl", nullptr, nullptr,
                          "main", "ps_5_0",
                          D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
                          &pixelShader, &errorBuff);
  if (FAILED(hr)) {
    OutputDebugStringA((char *)errorBuff->GetBufferPointer());
    return false;
  }
  D3D12_SHADER_BYTECODE pixelShaderBytecode = {
      .pShaderBytecode = pixelShader->GetBufferPointer(),
      .BytecodeLength = pixelShader->GetBufferSize(),
  };

  /* Create input layout */
  std::array<D3D12_INPUT_ELEMENT_DESC, 1> inputLayout = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 0}};

  D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {
      .pInputElementDescs = inputLayout.data(),
      .NumElements = inputLayout.size(),
  };

  /* Create pipeline state object */
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
      .pRootSignature = rootSignature,
      .VS = vertexShaderBytecode,
      .PS = pixelShaderBytecode,
      .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
      .SampleMask = 0xFFFFFFFF,
      .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
      .InputLayout = inputLayoutDesc,
      .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
      .NumRenderTargets = 1,
      .SampleDesc = sampleDesc,
  };
  psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM,
  hr = device->CreateGraphicsPipelineState(&psoDesc,
                                           IID_PPV_ARGS(&pipelineStateObject));
  RETURN_IF_FAILED(hr);

  /* Create vertex buffer */

  std::vector<Vertex> vertices = {
      {{0.0f, 0.5f, 0.5f}},
      {{0.5f, -0.5f, 0.5f}},
      {{-0.5f, -0.5f, 0.5f}},
  };

  // create default heap
  int vBufferSize = sizeof(vertices[0]) * vertices.size();
  auto commitedResourceHeapProperties =
      CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  auto resourceHeapDesc = CD3DX12_RESOURCE_DESC::Buffer(vBufferSize);
  device->CreateCommittedResource(
      &commitedResourceHeapProperties, D3D12_HEAP_FLAG_NONE, &resourceHeapDesc,
      D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&vertexBuffer));
  vertexBuffer->SetName(L"Vertex Buffer Resource Heap");

  // Create upload heap
  ID3D12Resource *vBufferUploadHeap;
  auto vBufferUploadHeapProperties =
      CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  auto vBufferUploadResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vBufferSize);
  device->CreateCommittedResource(
      &vBufferUploadHeapProperties, D3D12_HEAP_FLAG_NONE,
      &vBufferUploadResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
      IID_PPV_ARGS(&vBufferUploadHeap));
  vBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

  auto vertexData = D3D12_SUBRESOURCE_DATA{
      .pData = reinterpret_cast<BYTE *>(vertices.data()),
      .RowPitch = vBufferSize,
      .SlicePitch = vBufferSize,
  };
  UpdateSubresources(commandList, vertexBuffer, vBufferUploadHeap, 0, 0, 1,
                     &vertexData);
  auto vertexBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
      vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST,
      D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
  commandList->ResourceBarrier(1, &vertexBufferBarrier);

  // Upload initial assets
  commandList->Close();
  std::array<ID3D12CommandList *, 1> commandLists = {commandList};
  commandQueue->ExecuteCommandLists(commandLists.size(), commandLists.data());

  fenceValue[frameIndex]++;
  hr = commandQueue->Signal(fence[frameIndex], fenceValue[frameIndex]);
  STOP_RUNNING_IF_FAILED(hr);

  vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
  vertexBufferView.StrideInBytes = sizeof(Vertex);
  vertexBufferView.SizeInBytes = vBufferSize;

  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  viewport.Width = initInfo.width;
  viewport.Height = initInfo.height;
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;

  scissorRect.left = 0;
  scissorRect.top = 0;
  scissorRect.right = initInfo.width;
  scissorRect.bottom = initInfo.height;

#undef RETURN_IF_FAILED
#undef STOP_RUNNING_IF_FAILED
  return true;
}

void Renderer::Update() {}

void Renderer::UpdatePipeline() {
  HRESULT hr;
  WaitForPreviousFrame();
  hr = commandAllocator[frameIndex]->Reset();
  if (FAILED(hr)) {
    Running = false;
  }
  hr = commandList->Reset(commandAllocator[frameIndex], pipelineStateObject);
  if (FAILED(hr)) {
    Running = false;
  }

  auto resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
      renderTargets[frameIndex], D3D12_RESOURCE_STATE_PRESENT,
      D3D12_RESOURCE_STATE_RENDER_TARGET);
  commandList->ResourceBarrier(1, &resourceBarrier);

  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
      rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex,
      rtvDescriptorSize);
  commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

  // Clear the render target by using the ClearRenderTargetView command
  const float clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
  commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

  // draw triangle
  commandList->SetGraphicsRootSignature(
      rootSignature);                              // set the root signature
  commandList->RSSetViewports(1, &viewport);       // set the viewports
  commandList->RSSetScissorRects(1, &scissorRect); // set the scissor rects
  commandList->IASetPrimitiveTopology(
      D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // set the primitive topology
  commandList->IASetVertexBuffers(
      0, 1, &vertexBufferView); // set the vertex buffer (using the vertex
                                // buffer view)
  commandList->DrawInstanced(3, 1, 0,
                             0); // finally draw 3 vertices (draw the triangle)

  // transition the "frameIndex" render target from the render target state to
  // the present state. If the debug layer is enabled, you will receive a
  // warning if present is called on the render target when it's not in the
  // present state
  resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
      renderTargets[frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET,
      D3D12_RESOURCE_STATE_PRESENT);
  commandList->ResourceBarrier(1, &resourceBarrier);

  hr = commandList->Close();
  if (FAILED(hr)) {
    Running = false;
  }
}

void Renderer::Render() {
  HRESULT hr;
  UpdatePipeline();
  std::array<ID3D12CommandList *, 1> commandLists = {commandList};
  commandQueue->ExecuteCommandLists(commandLists.size(), commandLists.data());
  hr = commandQueue->Signal(fence[frameIndex], fenceValue[frameIndex]);
  if (FAILED(hr)) {
    Running = false;
  }

  hr = swapChain->Present(0, 0);
  if (FAILED(hr)) {
    Running = false;
  }
}

void Renderer::Cleanup() {
  for (auto i = 0; i < frameBufferCount; i++) {
    frameIndex = i;
    WaitForPreviousFrame();
  }
  BOOL fs = false;
  if (swapChain->GetFullscreenState(&fs, NULL)) {
    swapChain->SetFullscreenState(false, NULL);
  }

  SAFE_RELEASE(device);
  SAFE_RELEASE(swapChain);
  SAFE_RELEASE(commandQueue);
  SAFE_RELEASE(rtvDescriptorHeap);
  SAFE_RELEASE(commandList);

  for (auto i = 0; i < frameBufferCount; i++) {
    SAFE_RELEASE(renderTargets[i]);
    SAFE_RELEASE(commandAllocator[i]);
    SAFE_RELEASE(fence[i]);
  }
  SAFE_RELEASE(pipelineStateObject);
  SAFE_RELEASE(rootSignature);
  SAFE_RELEASE(vertexBuffer);
}

void Renderer::WaitForPreviousFrame() {
  HRESULT hr;
  frameIndex = swapChain->GetCurrentBackBufferIndex();
  if (fence[frameIndex]->GetCompletedValue() < fenceValue[frameIndex]) {
    hr = fence[frameIndex]->SetEventOnCompletion(fenceValue[frameIndex],
                                                 fenceEvent);
    if (FAILED(hr)) {
      Running = false;
    }

    WaitForSingleObject(fenceEvent, INFINITE);
  }
  fenceValue[frameIndex]++;
}

void Renderer::Close() {
  WaitForPreviousFrame();
  CloseHandle(fenceEvent);
  Cleanup();
}
