//
// Created by arrayJY on 2023/04/16.
//

#include "cloth_renderer.h"
#include <iostream>
#include <stdexcept>
#include <tiny_obj_loader.h>

void ClothRenderer::InitDirectX(const InitInfo& initInfo)
{
  Renderer::InitDirectX(initInfo);

  LoadCloth();
  CreateRootSignature();
  CreateClothRootSignature();
  CreateDescriptorHeaps();
  CreateShadersAndInputLayout();
  CreateRenderItems();
  CreateFrameResources();
  CreatePSOs();
}
void ClothRenderer::OnResize(UINT width, UINT height) {}
void ClothRenderer::Update(const GameTimer& timer) {}
void ClothRenderer::Draw(const GameTimer& timer) {}

void ClothRenderer::LoadCloth()
{
  std::string inputfile = MODEL_DIR "/cloth.obj";
  tinyobj::ObjReaderConfig reader_config;
  reader_config.mtl_search_path = "./";
  tinyobj::ObjReader reader;

  if (!reader.ParseFromFile(inputfile, reader_config)) {
    if (!reader.Error().empty()) {
      throw(std::runtime_error("TinyObjReader: " + reader.Error()));
    }
  }

  if (!reader.Warning().empty()) {
    std::cout << "TinyObjReader: " << reader.Warning();
  }

  auto& attrib = reader.GetAttrib();
  auto& shapes = reader.GetShapes();
  auto& materials = reader.GetMaterials();

  assert(shapes.size() == 1);

  Vertecies.reserve(shapes[0].mesh.indices.size() / 3);
  for (const auto& index : shapes[0].mesh.indices) {
    Vertex vertex;

    vertex.Pos.x = attrib.vertices[3 * index.vertex_index + 0];
    vertex.Pos.y = attrib.vertices[3 * index.vertex_index + 1];
    vertex.Pos.z = attrib.vertices[3 * index.vertex_index + 2];

    vertex.Normal.x = attrib.normals[3 * index.normal_index + 0];
    vertex.Normal.y = attrib.normals[3 * index.normal_index + 1];
    vertex.Normal.z = attrib.normals[3 * index.normal_index + 2];

    Vertecies.push_back(vertex);
  }
}

void ClothRenderer::CreateRootSignature()
{
  CD3DX12_ROOT_PARAMETER slotRootParameter[3];

  slotRootParameter[0].InitAsConstantBufferView(0);
  slotRootParameter[1].InitAsConstantBufferView(1);
  slotRootParameter[2].InitAsConstantBufferView(2);

  CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
    3,
    slotRootParameter,
    0,
    nullptr,
    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  ComPtr<ID3DBlob> serializedRootSig = nullptr;
  ComPtr<ID3DBlob> errorBlob = nullptr;
  HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc,
                                           D3D_ROOT_SIGNATURE_VERSION_1,
                                           serializedRootSig.GetAddressOf(),
                                           errorBlob.GetAddressOf());
  if (errorBlob != nullptr) {
    ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
  }
  ThrowIfFailed(hr);

  ThrowIfFailed(
    device->CreateRootSignature(0,
                                serializedRootSig->GetBufferPointer(),
                                serializedRootSig->GetBufferSize(),
                                IID_PPV_ARGS(RootSignature.GetAddressOf())));
}

void ClothRenderer::CreateClothRootSignature()
{
  CD3DX12_DESCRIPTOR_RANGE srvTable;
  CD3DX12_DESCRIPTOR_RANGE uavTable;

  srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
  uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

  CD3DX12_ROOT_PARAMETER slotRootParameter[2];
  slotRootParameter[0].InitAsDescriptorTable(1, &srvTable);
  slotRootParameter[1].InitAsDescriptorTable(1, &uavTable);

  CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
    2,
    slotRootParameter,
    0,
    nullptr,
    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  ComPtr<ID3DBlob> serializedRootSig = nullptr;
  ComPtr<ID3DBlob> errorBlob = nullptr;
  HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc,
                                           D3D_ROOT_SIGNATURE_VERSION_1,
                                           serializedRootSig.GetAddressOf(),
                                           errorBlob.GetAddressOf());

  if (errorBlob != nullptr) {
    ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
  }
  ThrowIfFailed(hr);

  ThrowIfFailed(device->CreateRootSignature(
    0,
    serializedRootSig->GetBufferPointer(),
    serializedRootSig->GetBufferSize(),
    IID_PPV_ARGS(ClothRootSignature.GetAddressOf())));
}

void ClothRenderer::CreateDescriptorHeaps()
{
  auto srvHeapDesc = D3D12_DESCRIPTOR_HEAP_DESC{
    .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
    .NumDescriptors = 2,
    .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
  };
  ThrowIfFailed(
    device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&SrvUavCbvHeap)));

  ClothSimulator->BuildDescriptors(
    CD3DX12_CPU_DESCRIPTOR_HANDLE(
      SrvUavCbvHeap->GetCPUDescriptorHandleForHeapStart(),
      0,
      cbvUavDescriptorSize),
    CD3DX12_GPU_DESCRIPTOR_HANDLE(
      SrvUavCbvHeap->GetGPUDescriptorHandleForHeapStart(),
      0,
      cbvUavDescriptorSize),
    cbvUavDescriptorSize);
}

void ClothRenderer::CreateRenderItems() {}

void ClothRenderer::CreateFrameResources()
{
  for (auto i = 0; i < FrameResourceCount; i++) {
    FrameResources.push_back(std::make_unique<FrameResource>(
      device.Get(), 1, (UINT)AllRenderItems.size()));
  }
}

void ClothRenderer::CreateShadersAndInputLayout()
{
  Shaders["mainVS"] =
    DXUtils::CompileShader(SHADER_DIR L"/main.hlsl", nullptr, "VS", "vs_5_1");
  Shaders["mainPS"] =
    DXUtils::CompileShader(SHADER_DIR L"/main.hlsl", nullptr, "PS", "ps_5_1");
  Shaders["clothCS"] =
    DXUtils::CompileShader(SHADER_DIR L"/cloth.hlsl", nullptr, "CS", "cs_5_0");

  InputLayouts = { D3D12_INPUT_ELEMENT_DESC{
    .SemanticName = "INDEX",
    .SemanticIndex = 0,
    .Format = DXGI_FORMAT_R32_UINT,
    .InputSlot = 0,
    .AlignedByteOffset = 0,
    .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
    .InstanceDataStepRate = 0,
  } };
}

void ClothRenderer::CreatePSOs()
{
  D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {
    .pRootSignature = RootSignature.Get(),
    .CS = CD3DX12_SHADER_BYTECODE(Shaders["ClothCS"].Get()),
    .Flags = D3D12_PIPELINE_STATE_FLAG_NONE,
  };
  ThrowIfFailed(device->CreateComputePipelineState(
    &computePsoDesc, IID_PPV_ARGS(PSOs["ClothCS"].GetAddressOf())));

  D3D12_GRAPHICS_PIPELINE_STATE_DESC mainPsoDesc = {
      .pRootSignature = RootSignature.Get(),
      .VS = CD3DX12_SHADER_BYTECODE(Shaders["mainVS"].Get()),
      .PS = CD3DX12_SHADER_BYTECODE(Shaders["mainPS"].Get()),
      .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
      .SampleMask = UINT_MAX,
      .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
      .DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
      .InputLayout = { InputLayouts.data(), (UINT)InputLayouts.size() },
      .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
      .NumRenderTargets = 1,
      .RTVFormats = { backBufferFormat },
      .DSVFormat = depthStencilFormat,
      .SampleDesc = {
        .Count = msaaState ? 4U : 1U,
        .Quality = msaaState ? (msaaQuality - 1) : 0U,
      },
    };
  ThrowIfFailed(device->CreateGraphicsPipelineState(
    &mainPsoDesc, IID_PPV_ARGS(PSOs["main"].GetAddressOf())));
}
