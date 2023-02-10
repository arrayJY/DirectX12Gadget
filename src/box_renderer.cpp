//
// Created by arrayJY on 2023/02/08.
//

#include "box_renderer.h"
#include <array>

void BoxRenderer::InitDirectX(const InitInfo &initInfo) {
  Renderer::InitDirectX(initInfo);
  OnResize(initInfo.width, initInfo.height);

  ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));

  BuildDescriptorHeaps();
  CreateConstantBuffer();
  CreateRootSignature();
  CreateShadersAndInputLayout();
  CreateBoxGeometry();
  CreatePSO();

  ThrowIfFailed(commandList->Close());
  ID3D12CommandList *cmdsLists[] = {commandList.Get()};
  commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

  FlushCommandQueue();
}

void BoxRenderer::BuildDescriptorHeaps() {
  auto cbvHeapDesc = D3D12_DESCRIPTOR_HEAP_DESC{
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = 1,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
      .NodeMask = 0,
  };
  ThrowIfFailed(
      device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&cbvHeap)));
}

void BoxRenderer::CreateConstantBuffer() {
  uploadBuffer =
      std::make_unique<UploadBuffer<ConstantObject>>(device.Get(), 1);
  UINT constantStructSize =
      DXUtils::CalcConstantBufferSize(sizeof(ConstantObject));
  auto constantBufferAddress = uploadBuffer->Resource()->GetGPUVirtualAddress();
  auto constantBufferViewDesc = D3D12_CONSTANT_BUFFER_VIEW_DESC{
      .BufferLocation = constantBufferAddress,
      .SizeInBytes = constantStructSize,
  };
  device->CreateConstantBufferView(
      &constantBufferViewDesc, cbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void BoxRenderer::CreateRootSignature() {
  CD3DX12_ROOT_PARAMETER slotRootParameter[1];
  CD3DX12_DESCRIPTOR_RANGE cbvTable;

  cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
  slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

  auto rootSignDesc = CD3DX12_ROOT_SIGNATURE_DESC{
      1, slotRootParameter, 0, nullptr,
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT};

  ComPtr<ID3DBlob> serializeRootSignature = nullptr;
  ComPtr<ID3DBlob> errorBlob = nullptr;

  D3D12SerializeRootSignature(&rootSignDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                              serializeRootSignature.GetAddressOf(),
                              errorBlob.GetAddressOf());

  ThrowIfFailed(device->CreateRootSignature(
      0, serializeRootSignature->GetBufferPointer(),
      serializeRootSignature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}

void BoxRenderer::CreateShadersAndInputLayout() {
  vertexShader = DXUtils::CompileShader(SHADER_DIR L"/Color.hlsl", nullptr,
                                        "VS", "vs_5_0");
  pixelShader = DXUtils::CompileShader(SHADER_DIR L"/Color.hlsl", nullptr, "PS",
                                       "ps_5_0");
  inputLayout.push_back(D3D12_INPUT_ELEMENT_DESC{
      .SemanticName = "POSITION",
      .SemanticIndex = 0,
      .Format = DXGI_FORMAT_R32G32B32_FLOAT,
      .InputSlot = 0,
      .AlignedByteOffset = 0,
      .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
      .InstanceDataStepRate = 0});

  inputLayout.push_back(D3D12_INPUT_ELEMENT_DESC{
      .SemanticName = "COLOR",
      .SemanticIndex = 0,
      .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
      .InputSlot = 0,
      .AlignedByteOffset = 12,
      .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
      .InstanceDataStepRate = 0});
}

void BoxRenderer::CreateBoxGeometry() {
  std::array<Vertex, 8> vertices = {
      Vertex{XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(DirectX::Colors::White)},
      Vertex{XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(DirectX::Colors::Black)},
      Vertex{XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(DirectX::Colors::Red)},
      Vertex{XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(DirectX::Colors::Green)},
      Vertex{XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(DirectX::Colors::Blue)},
      Vertex{XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(DirectX::Colors::Yellow)},
      Vertex{XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(DirectX::Colors::Cyan)},
      Vertex{XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(DirectX::Colors::Magenta)},
  };

  std::array<std::uint16_t, 36> indices = {
      0, 1, 2, 0, 2, 3, 4, 6, 5, 4, 7, 6, 4, 5, 1, 4, 1, 0,
      3, 2, 6, 3, 6, 7, 1, 5, 6, 1, 6, 2, 4, 0, 3, 4, 3, 7,
  };

  const UINT vertexBufferByteSize = vertices.size() * sizeof(Vertex);
  const UINT indexBufferByteSize = indices.size() * sizeof(std::uint16_t);

  BoxGeometry = std::make_unique<MeshGeometry>();
  BoxGeometry->Name = "BoxGemometry";

  ThrowIfFailed(
      D3DCreateBlob(vertexBufferByteSize, &BoxGeometry->VertexCPUBuffer));
  CopyMemory(BoxGeometry->VertexCPUBuffer->GetBufferPointer(), vertices.data(),
             vertexBufferByteSize);

  ThrowIfFailed(
      D3DCreateBlob(indexBufferByteSize, &BoxGeometry->IndexCPUBuffer));
  CopyMemory(BoxGeometry->IndexCPUBuffer->GetBufferPointer(), indices.data(),
             indexBufferByteSize);

  BoxGeometry->VertexGPUBuffer = DXUtils::CreateDefaultBuffer(
      device.Get(), commandList.Get(), vertices.data(), vertexBufferByteSize,
      BoxGeometry->VertexBufferUploader);

  BoxGeometry->IndexGPUBuffer = DXUtils::CreateDefaultBuffer(
      device.Get(), commandList.Get(), indices.data(), indexBufferByteSize,
      BoxGeometry->IndexBufferUploader);

  BoxGeometry->VertexByteStride = sizeof(Vertex);
  BoxGeometry->VertexBufferByteSize = vertexBufferByteSize;
  BoxGeometry->IndexFormat = DXGI_FORMAT_R16_UINT;
  BoxGeometry->IndexBufferByteSize = indexBufferByteSize;

  BoxGeometry->DrawArgs["box"] = SubmeshGeometry{
      .IndexCount = indices.size(),
      .StartIndexLocation = 0,
      .BaseVertexLocation = 0,
  };
}

void BoxRenderer::CreatePSO() {
  auto psoDesc = D3D12_GRAPHICS_PIPELINE_STATE_DESC{
      .pRootSignature = rootSignature.Get(),
      .VS =
          D3D12_SHADER_BYTECODE{
              .pShaderBytecode =
                  reinterpret_cast<BYTE *>(vertexShader->GetBufferPointer()),
              .BytecodeLength = vertexShader->GetBufferSize(),
          },
      .PS =
          D3D12_SHADER_BYTECODE{
              .pShaderBytecode =
                  reinterpret_cast<BYTE *>(pixelShader->GetBufferPointer()),
              .BytecodeLength = pixelShader->GetBufferSize(),
          },
      .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
      .SampleMask = UINT_MAX,
      .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
      .DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
      .InputLayout = {.pInputElementDescs = inputLayout.data(),
                      .NumElements = (UINT)inputLayout.size()},
      .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
      .NumRenderTargets = 1,
      .RTVFormats = {backBufferFormat},
      .DSVFormat = depthStencilFormat,
      .SampleDesc =
          DXGI_SAMPLE_DESC{.Count = msaaState ? 4U : 1U,
                           .Quality = msaaState ? (msaaQuality - 1) : 0},

  };
  ThrowIfFailed(
      device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&PSO)))
}

void BoxRenderer::Draw(const GameTimer &timer) {
  ThrowIfFailed(commandAllocator->Reset());

  ThrowIfFailed(commandList->Reset(commandAllocator.Get(), PSO.Get()));

  commandList->RSSetViewports(1, &viewport);
  commandList->RSSetScissorRects(1, &scissorRect);

  auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
      CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT,
      D3D12_RESOURCE_STATE_RENDER_TARGET);
  commandList->ResourceBarrier(1, &barrier);

  commandList->ClearRenderTargetView(
      CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);
  commandList->ClearDepthStencilView(
      DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
      1.0f, 0, 0, nullptr);

  auto currentBackBufferView = CurrentBackBufferView();
  auto depthStencilView = DepthStencilView();
  commandList->OMSetRenderTargets(1, &currentBackBufferView, true,
                                  &depthStencilView);

  ID3D12DescriptorHeap *descriptorHeaps[] = {cbvHeap.Get()};
  commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

  commandList->SetGraphicsRootSignature(rootSignature.Get());

  auto vertexBufferView = BoxGeometry->VertexBufferView();
  commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
  auto indexBufferView = BoxGeometry->IndexBufferView();
  commandList->IASetIndexBuffer(&indexBufferView);

  commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  commandList->SetGraphicsRootDescriptorTable(
      0, cbvHeap->GetGPUDescriptorHandleForHeapStart());

  commandList->DrawIndexedInstanced(BoxGeometry->DrawArgs["box"].IndexCount, 1,
                                    0, 0, 0);

  auto barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
      CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET,
      D3D12_RESOURCE_STATE_PRESENT);
  commandList->ResourceBarrier(1, &barrier2);

  ThrowIfFailed(commandList->Close());

  ID3D12CommandList *cmdsLists[] = {commandList.Get()};
  commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

  ThrowIfFailed(swapChain->Present(0, 0));
  currentBackBufferIndex = (currentBackBufferIndex + 1) % swapChainBufferCount;

  FlushCommandQueue();
}

void BoxRenderer::Update(const GameTimer &timer) {
  // Convert Spherical to Cartesian coordinates.
  float x = radius * sinf(phi) * cosf(theta);
  float z = radius * sinf(phi) * sinf(theta);
  float y = radius * cosf(phi);

  // Build the view matrix.
  XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
  XMVECTOR target = XMVectorZero();
  XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

  XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
  XMStoreFloat4x4(&viewMatrix, view);

  XMMATRIX world = XMLoadFloat4x4(&worldMatrix);
  XMMATRIX proj = XMLoadFloat4x4(&projectionMatrix);
  XMMATRIX worldViewProj = world * view * proj;

  // Update the constant buffer with the latest worldViewProj matrix.
  ConstantObject objConstants;
  XMStoreFloat4x4(&objConstants.MVP, XMMatrixTranspose(worldViewProj));
  uploadBuffer->CopyData(0, objConstants);
}

void BoxRenderer::OnResize(UINT width, UINT height) {
  Renderer::OnResize(width, height);
  XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(
      0.25f * MathHelper::PI, (float)width / height, 1.0f, 1000.0f);
  XMStoreFloat4x4(&projectionMatrix, P);
}
