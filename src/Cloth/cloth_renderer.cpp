//
// Created by arrayJY on 2023/04/16.
//

#include "cloth_renderer.h"

void ClothRenderer::InitDirectX(const InitInfo &initInfo) {}
void ClothRenderer::OnResize(UINT width, UINT height) {}
void ClothRenderer::Update(const GameTimer &timer) {}
void ClothRenderer::Draw(const GameTimer &timer) {}

void ClothRenderer::CreateBuffers() {
  // Input Buffer
  UINT byteSize = sizeof(Vertex) * Vertecies.size();
  VertexInputBuffer = DXUtils::CreateDefaultBuffer(
      device.Get(), commandList.Get(), Vertecies.data(), byteSize,
      VertexInputUploadBuffer);

  // Output Buffer
  auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(
      byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
  ThrowIfFailed(device->CreateCommittedResource(
      &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
      D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
      IID_PPV_ARGS(&VertexOutputBuffer)));
}

void ClothRenderer::CreateRootSignature() {
  CD3DX12_ROOT_PARAMETER slotRootParameter[2];

  slotRootParameter[0].InitAsShaderResourceView(0);
  slotRootParameter[1].InitAsUnorderedAccessView(0);

  CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
      2, slotRootParameter, 0, nullptr,
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

void ClothRenderer::CreateShadersAndInputLayout() {
  Shaders["ClothCS"] =
      DXUtils::CompileShader(L"shaders\\cloth.hlsl", nullptr, "CS", "cs_5_0");
}

void ClothRenderer::CreatePSOs() {
  D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {
      .pRootSignature = RootSignature.Get(),
      .CS = CD3DX12_SHADER_BYTECODE(Shaders["ClothCS"].Get()),
      .Flags = D3D12_PIPELINE_STATE_FLAG_NONE,
  };
  ThrowIfFailed(device->CreateComputePipelineState(
      &computePsoDesc, IID_PPV_ARGS(&PSOs["ClothCS"])));
}

void ClothRenderer::DoCompute() {
  ThrowIfFailed(commandAllocator->Reset());
  ThrowIfFailed(
      commandList->Reset(commandAllocator.Get(), PSOs["ClothCS"].Get()));

  commandList->SetComputeRootSignature(RootSignature.Get());

  commandList->SetComputeRootShaderResourceView(
      0, VertexInputBuffer->GetGPUVirtualAddress());
  commandList->SetComputeRootUnorderedAccessView(
      1, VertexOutputBuffer->GetGPUVirtualAddress());

  commandList->Dispatch(1, 1, 1);

  auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
      VertexOutputBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
      D3D12_RESOURCE_STATE_COPY_SOURCE);
  commandList->ResourceBarrier(1, &barrier);

  ThrowIfFailed(commandList->Close());

  ID3D12CommandList *cmdsLists[] = {commandList.Get()};
  commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
}
