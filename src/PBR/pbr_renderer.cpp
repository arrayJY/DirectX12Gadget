//
// Created by arrayJY on 2023/02/14.
//

#include "pbr_renderer.h"
#include "geometry_generator.h"
using namespace DirectX;

void PBRRenderer::InitDirectX(const InitInfo &initInfo) {
  Renderer::InitDirectX(initInfo);

  ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));
}

void PBRRenderer::Update(const GameTimer &timer) {
  CurrentFrameResourceIndex =
      (CurrentFrameResourceIndex + 1) % FrameResourceCount;
  CurrentFrameResource = FrameResources[CurrentFrameResourceIndex].get();

  if (CurrentFrameResource->Fence != 0 &&
      fence->GetCompletedValue() < CurrentFrameResource->Fence) {
    WaitForFence(CurrentFrameResource->Fence);
  }

  UpdateObjectConstantsBuffer(timer);
  UpdateMainPassConstantsBuffer(timer);
}

void PBRRenderer::Draw(const GameTimer &timer) {}

void PBRRenderer::CreateRootSignature() {
  CD3DX12_DESCRIPTOR_RANGE cbvTable[2];
  cbvTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
  cbvTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

  CD3DX12_ROOT_PARAMETER slotRootParameter[2];
  slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable[0]);
  slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable[1]);

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

void PBRRenderer::CreateShaderAndInputLayout() {
  Shaders["standardVS"] = DXUtils::CompileShader(SHADER_DIR L"/color.hlsl",
                                                 nullptr, "VS", "vs_5_1");
  Shaders["opaquePS"] = DXUtils::CompileShader(SHADER_DIR L"/color.hlsl",
                                               nullptr, "PS", "ps_5_1");

  InputLayout.push_back(D3D12_INPUT_ELEMENT_DESC{
      .SemanticName = "POSITION",
      .SemanticIndex = 0,
      .Format = DXGI_FORMAT_R32G32B32_FLOAT,
      .InputSlot = 0,
      .AlignedByteOffset = 0,
      .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
      .InstanceDataStepRate = 0});
  InputLayout.push_back(D3D12_INPUT_ELEMENT_DESC{
      .SemanticName = "COLOR",
      .SemanticIndex = 0,
      .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
      .InputSlot = 0,
      .AlignedByteOffset = 12,
      .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
      .InstanceDataStepRate = 0});
}

void PBRRenderer::CreateShapeGeometry() {
  GeometryGenerator geoGen;
  GeometryGenerator::MeshData box = geoGen.CreateBox(1.5f, 0.5f, 1.5f, 3);
  GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
  GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
  GeometryGenerator::MeshData cylinder =
      geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

  //
  // We are concatenating all the geometry into one big vertex/index buffer.  So
  // define the regions in the buffer each submesh covers.
  //

  // Cache the vertex offsets to each object in the concatenated vertex buffer.
  UINT boxVertexOffset = 0;
  UINT gridVertexOffset = (UINT)box.Vertices.size();
  UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
  UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();

  // Cache the starting index for each object in the concatenated index buffer.
  UINT boxIndexOffset = 0;
  UINT gridIndexOffset = (UINT)box.Indices32.size();
  UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
  UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();

  // Define the SubmeshGeometry that cover different
  // regions of the vertex/index buffers.

  SubmeshGeometry boxSubmesh;
  boxSubmesh.IndexCount = (UINT)box.Indices32.size();
  boxSubmesh.StartIndexLocation = boxIndexOffset;
  boxSubmesh.BaseVertexLocation = boxVertexOffset;

  SubmeshGeometry gridSubmesh;
  gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
  gridSubmesh.StartIndexLocation = gridIndexOffset;
  gridSubmesh.BaseVertexLocation = gridVertexOffset;

  SubmeshGeometry sphereSubmesh;
  sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
  sphereSubmesh.StartIndexLocation = sphereIndexOffset;
  sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

  SubmeshGeometry cylinderSubmesh;
  cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
  cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
  cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;

  //
  // Extract the vertex elements we are interested in and pack the
  // vertices of all the meshes into one vertex buffer.
  //

  auto totalVertexCount = box.Vertices.size() + grid.Vertices.size() +
                          sphere.Vertices.size() + cylinder.Vertices.size();

  std::vector<Vertex> vertices(totalVertexCount);

  UINT k = 0;
  for (size_t i = 0; i < box.Vertices.size(); ++i, ++k) {
    vertices[k].Pos = box.Vertices[i].Position;
    vertices[k].Color = XMFLOAT4(DirectX::Colors::DarkGreen);
  }

  for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k) {
    vertices[k].Pos = grid.Vertices[i].Position;
    vertices[k].Color = XMFLOAT4(DirectX::Colors::ForestGreen);
  }

  for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k) {
    vertices[k].Pos = sphere.Vertices[i].Position;
    vertices[k].Color = XMFLOAT4(DirectX::Colors::Crimson);
  }

  for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k) {
    vertices[k].Pos = cylinder.Vertices[i].Position;
    vertices[k].Color = XMFLOAT4(DirectX::Colors::SteelBlue);
  }

  std::vector<std::uint16_t> indices;
  indices.insert(indices.end(), std::begin(box.GetIndices16()),
                 std::end(box.GetIndices16()));
  indices.insert(indices.end(), std::begin(grid.GetIndices16()),
                 std::end(grid.GetIndices16()));
  indices.insert(indices.end(), std::begin(sphere.GetIndices16()),
                 std::end(sphere.GetIndices16()));
  indices.insert(indices.end(), std::begin(cylinder.GetIndices16()),
                 std::end(cylinder.GetIndices16()));

  const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
  const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

  auto geo = std::make_unique<MeshGeometry>();
  geo->Name = "shapeGeo";

  ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexCPUBuffer));
  CopyMemory(geo->VertexCPUBuffer->GetBufferPointer(), vertices.data(),
             vbByteSize);

  ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexCPUBuffer));
  CopyMemory(geo->IndexCPUBuffer->GetBufferPointer(), indices.data(),
             ibByteSize);

  geo->VertexGPUBuffer = DXUtils::CreateDefaultBuffer(
      device.Get(), commandList.Get(), vertices.data(), vbByteSize,
      geo->VertexBufferUploader);

  geo->IndexGPUBuffer = DXUtils::CreateDefaultBuffer(
      device.Get(), commandList.Get(), indices.data(), ibByteSize,
      geo->IndexBufferUploader);

  geo->VertexByteStride = sizeof(Vertex);
  geo->VertexBufferByteSize = vbByteSize;
  geo->IndexFormat = DXGI_FORMAT_R16_UINT;
  geo->IndexBufferByteSize = ibByteSize;

  geo->DrawArgs["box"] = boxSubmesh;
  geo->DrawArgs["grid"] = gridSubmesh;
  geo->DrawArgs["sphere"] = sphereSubmesh;
  geo->DrawArgs["cylinder"] = cylinderSubmesh;

  Geometries[geo->Name] = std::move(geo);
}

void PBRRenderer::CreateRenderItems() {}

void PBRRenderer::CreateFrameResource() {}

void PBRRenderer::CreateDescriptorHeaps() {}

void PBRRenderer::CreateConstantBufferView() {}

void PBRRenderer::CreatePSOs() {}

void PBRRenderer::UpdateObjectConstantsBuffer(const GameTimer &timer) {
  auto currentObjectCB = CurrentFrameResource->ObjectConstantsBuffer.get();
  for (auto &i : AllRenderItems) {
    if (i->NumberFramesDirty > 0) {
      auto world = DirectX::XMLoadFloat4x4(&i->World);

      ObjectConstants objConstants;
      DirectX::XMStoreFloat4x4(&objConstants.World,
                               DirectX::XMMatrixTranspose(world));

      currentObjectCB->CopyData(i->ObjectCBIndex, objConstants);
      i->NumberFramesDirty--;
    }
  }
}

void PBRRenderer::UpdateMainPassConstantsBuffer(const GameTimer &timer) {
  auto view = DirectX::XMLoadFloat4x4(&ViewMatrix);
  auto proj = DirectX::XMLoadFloat4x4(&ProjectionMatrix);

  auto viewProj = XMMatrixMultiply(view, proj);

  auto viewDeterminant = XMMatrixDeterminant(view);
  auto invView = XMMatrixInverse(&viewDeterminant, view);

  auto projDeterminant = XMMatrixDeterminant(proj);
  auto invProj = XMMatrixInverse(&projDeterminant, proj);

  auto viewProjDeterminant = XMMatrixDeterminant(viewProj);
  auto invViewProj = XMMatrixInverse(&viewProjDeterminant, viewProj);

  XMStoreFloat4x4(&MainPassConstants.View, XMMatrixTranspose(view));
  XMStoreFloat4x4(&MainPassConstants.InvView, XMMatrixTranspose(invView));
  XMStoreFloat4x4(&MainPassConstants.Proj, XMMatrixTranspose(proj));
  XMStoreFloat4x4(&MainPassConstants.InvProj, XMMatrixTranspose(invProj));
  XMStoreFloat4x4(&MainPassConstants.ViewProj, XMMatrixTranspose(viewProj));
  XMStoreFloat4x4(&MainPassConstants.InvViewProj,
                  XMMatrixTranspose(invViewProj));

  MainPassConstants.EyePosW = EyePos;
  MainPassConstants.RenderTargetSize = XMFLOAT2(Width, Height);
  MainPassConstants.InvRenderTargetSize = XMFLOAT2(1.0f / Width, 1.0f / Height);
  MainPassConstants.NearZ = 1.0f;
  MainPassConstants.FarZ = 1000.0f;
  MainPassConstants.TotalTime = timer.TotalTime();
  MainPassConstants.DeltaTime = timer.DeltaTime();

  CurrentFrameResource->PassConstantsBuffer->CopyData(0, MainPassConstants);
}
