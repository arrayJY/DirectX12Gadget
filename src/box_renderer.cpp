//
// Created by arrayJY on 2023/02/08.
//

#include "box_renderer.h"
#include "dx_utils.h"
#include "stdafx.h"
#include "upload_buffer.h"

void BoxRenderer::CreateConstantBuffer() {
  UploadBuffer<ConstantObject> uploadBuffer(device.Get(), 1);
  uploadBuffer.Upload();
  UINT constantStructSize =
      DXUtils::CalcConstantBufferSize(sizeof(ConstantObject));
  auto constantBufferAddress = uploadBuffer.Resource()->GetGPUVirtualAddress();
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
  vertexShader = DXUtils::CompileShader(SHADER_DIR L"VertexShader", nullptr,
                                        "VS", "vs_5_0");
  pixelShader = DXUtils::CompileShader(SHADER_DIR L"PixelShader", nullptr, "PS",
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
