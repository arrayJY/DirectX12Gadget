//
// Created by arrayJY on 2023/02/14.
//

#include "pbr_renderer.h"
#include "geometry_generator.h"
#include "render_item.h"

using namespace DirectX;

void PBRRenderer::InitDirectX(const InitInfo &initInfo) {
  Renderer::InitDirectX(initInfo);

  ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));

  CreateRootSignature();
  CreateShaderAndInputLayout();
  CreateShapeGeometry();
  CreateRenderItems();
  CreateFrameResource();
  CreateDescriptorHeaps();
  CreateConstantBufferView();
  CreatePSOs();

  ThrowIfFailed(commandList->Close());
  ID3D12CommandList *cmdsLists[] = {commandList.Get()};
  commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

  FlushCommandQueue();
}

void PBRRenderer::Update(const GameTimer &timer) {
  UpdateCamera(timer);

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

void PBRRenderer::OnResize(UINT width, UINT height) {
  Renderer::OnResize(width, height);
  XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::PI, AspectRatio(),
                                        1.0f, 1000.0f);
  XMStoreFloat4x4(&ProjectionMatrix, P);
}

void PBRRenderer::Draw(const GameTimer &timer) {
  auto cmdListAllocator = CurrentFrameResource->CommandAllocator;

  ThrowIfFailed(cmdListAllocator->Reset());

  if (IsWireFrame) {
    ThrowIfFailed(commandList->Reset(cmdListAllocator.Get(),
                                     PSOs["opaque_wireframe"].Get()))
  } else {
    ThrowIfFailed(
        commandList->Reset(cmdListAllocator.Get(), PSOs["opaque"].Get()))
  }

  commandList->RSSetViewports(1, &viewport);
  commandList->RSSetScissorRects(1, &scissorRect);

  auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
      CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT,
      D3D12_RESOURCE_STATE_RENDER_TARGET);
  commandList->ResourceBarrier(1, &barrier);

  commandList->ClearRenderTargetView(CurrentBackBufferView(),
                                     Colors::LightSteelBlue, 0, nullptr);
  commandList->ClearDepthStencilView(
      DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
      1.0f, 0, 0, nullptr);

  auto currentBackBufferView = CurrentBackBufferView();
  auto depthStencilView = DepthStencilView();
  commandList->OMSetRenderTargets(1, &currentBackBufferView, true,
                                  &depthStencilView);

  ID3D12DescriptorHeap *descriptorHeaps[] = {cbvHeap.Get()};
  commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

  commandList->SetGraphicsRootSignature(RootSignature.Get());

  int passCbvIndex = PassCbvOffset + CurrentFrameResourceIndex;
  auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
      cbvHeap->GetGPUDescriptorHandleForHeapStart());
  passCbvHandle.Offset(passCbvIndex, cbvUavDescriptorSize);
  commandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);

  DrawRenderItems(commandList.Get(), OpaqueRenderItems);

  const auto barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
      CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET,
      D3D12_RESOURCE_STATE_PRESENT);
  commandList->ResourceBarrier(1, &barrier2);

  ThrowIfFailed(commandList->Close());
  ID3D12CommandList *cmdsLists[] = {commandList.Get()};
  commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

  ThrowIfFailed(swapChain->Present(0, 0));
  currentBackBufferIndex = (currentBackBufferIndex + 1) % swapChainBufferCount;

  CurrentFrameResource->Fence = ++fenceValue;

  commandQueue->Signal(fence.Get(), fenceValue);
}

void PBRRenderer::DrawRenderItems(ID3D12GraphicsCommandList *cmdList,
                                  std::vector<RenderItem *> items) {
  UINT objCBByteSize = DXUtils::CalcConstantBufferSize(sizeof(ObjectConstants));
  auto objectCB = CurrentFrameResource->ObjectConstantsBuffer->Resource();

  for (auto i = 0; i < items.size(); i++) {
    auto renderItem = items[i];
    auto vertexBufferView = renderItem->Geometry->VertexBufferView();
    auto indexBufferView = renderItem->Geometry->IndexBufferView();

    cmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
    cmdList->IASetIndexBuffer(&indexBufferView);
    cmdList->IASetPrimitiveTopology(renderItem->PrimitiveType);

    UINT cbvIndex = CurrentFrameResourceIndex * (UINT)OpaqueRenderItems.size() +
                    renderItem->ObjectCBIndex;
    auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
        cbvHeap->GetGPUDescriptorHandleForHeapStart());
    cbvHandle.Offset(cbvIndex, cbvUavDescriptorSize);

    cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);
    cmdList->DrawIndexedInstanced(renderItem->IndexCount, 1,
                                  renderItem->StartIndexLocation,
                                  renderItem->BaseVertexLocation, 0);
  }
}

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

void PBRRenderer::CreateRenderItems() {
  auto BoxItem = std::make_unique<RenderItem>();
  XMStoreFloat4x4(&BoxItem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) *
                                       XMMatrixTranslation(0.0f, 0.5f, 0.0f));
  auto boxRitem = std::make_unique<RenderItem>();
  XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) *
                                        XMMatrixTranslation(0.0f, 0.5f, 0.0f));
  boxRitem->ObjectCBIndex = 0;
  boxRitem->Geometry = Geometries["shapeGeo"].get();
  boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  boxRitem->IndexCount = boxRitem->Geometry->DrawArgs["box"].IndexCount;
  boxRitem->StartIndexLocation =
      boxRitem->Geometry->DrawArgs["box"].StartIndexLocation;
  boxRitem->BaseVertexLocation =
      boxRitem->Geometry->DrawArgs["box"].BaseVertexLocation;
  AllRenderItems.push_back(std::move(boxRitem));

  auto gridRitem = std::make_unique<RenderItem>();
  gridRitem->World = MathHelper::Identity4x4();
  gridRitem->ObjectCBIndex = 1;
  gridRitem->Geometry = Geometries["shapeGeo"].get();
  gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  gridRitem->IndexCount = gridRitem->Geometry->DrawArgs["grid"].IndexCount;
  gridRitem->StartIndexLocation =
      gridRitem->Geometry->DrawArgs["grid"].StartIndexLocation;
  gridRitem->BaseVertexLocation =
      gridRitem->Geometry->DrawArgs["grid"].BaseVertexLocation;
  AllRenderItems.push_back(std::move(gridRitem));

  UINT ObjectCBIndex = 2;
  for (int i = 0; i < 5; ++i) {
    auto leftCylRitem = std::make_unique<RenderItem>();
    auto rightCylRitem = std::make_unique<RenderItem>();
    auto leftSphereRitem = std::make_unique<RenderItem>();
    auto rightSphereRitem = std::make_unique<RenderItem>();

    XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
    XMMATRIX rightCylWorld =
        XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);

    XMMATRIX leftSphereWorld =
        XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
    XMMATRIX rightSphereWorld =
        XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);

    XMStoreFloat4x4(&leftCylRitem->World, rightCylWorld);
    leftCylRitem->ObjectCBIndex = ObjectCBIndex++;
    leftCylRitem->Geometry = Geometries["shapeGeo"].get();
    leftCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    leftCylRitem->IndexCount =
        leftCylRitem->Geometry->DrawArgs["cylinder"].IndexCount;
    leftCylRitem->StartIndexLocation =
        leftCylRitem->Geometry->DrawArgs["cylinder"].StartIndexLocation;
    leftCylRitem->BaseVertexLocation =
        leftCylRitem->Geometry->DrawArgs["cylinder"].BaseVertexLocation;

    XMStoreFloat4x4(&rightCylRitem->World, leftCylWorld);
    rightCylRitem->ObjectCBIndex = ObjectCBIndex++;
    rightCylRitem->Geometry = Geometries["shapeGeo"].get();
    rightCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    rightCylRitem->IndexCount =
        rightCylRitem->Geometry->DrawArgs["cylinder"].IndexCount;
    rightCylRitem->StartIndexLocation =
        rightCylRitem->Geometry->DrawArgs["cylinder"].StartIndexLocation;
    rightCylRitem->BaseVertexLocation =
        rightCylRitem->Geometry->DrawArgs["cylinder"].BaseVertexLocation;

    XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
    leftSphereRitem->ObjectCBIndex = ObjectCBIndex++;
    leftSphereRitem->Geometry = Geometries["shapeGeo"].get();
    leftSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    leftSphereRitem->IndexCount =
        leftSphereRitem->Geometry->DrawArgs["sphere"].IndexCount;
    leftSphereRitem->StartIndexLocation =
        leftSphereRitem->Geometry->DrawArgs["sphere"].StartIndexLocation;
    leftSphereRitem->BaseVertexLocation =
        leftSphereRitem->Geometry->DrawArgs["sphere"].BaseVertexLocation;

    XMStoreFloat4x4(&rightSphereRitem->World, rightSphereWorld);
    rightSphereRitem->ObjectCBIndex = ObjectCBIndex++;
    rightSphereRitem->Geometry = Geometries["shapeGeo"].get();
    rightSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    rightSphereRitem->IndexCount =
        rightSphereRitem->Geometry->DrawArgs["sphere"].IndexCount;
    rightSphereRitem->StartIndexLocation =
        rightSphereRitem->Geometry->DrawArgs["sphere"].StartIndexLocation;
    rightSphereRitem->BaseVertexLocation =
        rightSphereRitem->Geometry->DrawArgs["sphere"].BaseVertexLocation;

    AllRenderItems.push_back(std::move(leftCylRitem));
    AllRenderItems.push_back(std::move(rightCylRitem));
    AllRenderItems.push_back(std::move(leftSphereRitem));
    AllRenderItems.push_back(std::move(rightSphereRitem));
  }

  // All the render items are opaque.
  for (auto &i : AllRenderItems) {
    OpaqueRenderItems.push_back(i.get());
  }
}

void PBRRenderer::CreateFrameResource() {
  FrameResources.reserve(FrameResourceCount);
  for (auto i = 0; i < FrameResourceCount; i++) {
    FrameResources.push_back(std::make_unique<FrameResource>(
        device.Get(), 1, AllRenderItems.size()));
  }
}

void PBRRenderer::CreateDescriptorHeaps() {
  auto objCount = OpaqueRenderItems.size();
  UINT descriptorCount = (objCount + 1) * FrameResourceCount;
  PassCbvOffset = objCount * FrameResourceCount;

  D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc{
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = descriptorCount,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
      .NodeMask = 0,
  };
  ThrowIfFailed(
      device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&cbvHeap)))
}

void PBRRenderer::CreateConstantBufferView() {
  UINT objCBByteSize = DXUtils::CalcConstantBufferSize(sizeof(ObjectConstants));
  UINT objCount = OpaqueRenderItems.size();

  for (auto frameIndex = 0; frameIndex < FrameResourceCount; frameIndex++) {
    auto objectCB =
        FrameResources[frameIndex]->ObjectConstantsBuffer->Resource();
    for (auto i = 0U; i < objCount; i++) {
      auto cbAddress = objectCB->GetGPUVirtualAddress();
      cbAddress += i * objCBByteSize;
      auto heapIndex = frameIndex * objCount + i;
      auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
          cbvHeap->GetCPUDescriptorHandleForHeapStart());
      handle.Offset(heapIndex, cbvUavDescriptorSize);

      D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{
          .BufferLocation = cbAddress,
          .SizeInBytes = objCBByteSize,
      };
      device->CreateConstantBufferView(&cbvDesc, handle);
    }
  }

  UINT passCBByteSize = DXUtils::CalcConstantBufferSize(sizeof(PassConstants));
  for (auto framIndex = 0; framIndex < FrameResourceCount; framIndex++) {
    auto passCB = FrameResources[framIndex]->PassConstantsBuffer->Resource();

    auto cbAddress = passCB->GetGPUVirtualAddress();
    auto heapIndex = PassCbvOffset + framIndex;

    auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
        cbvHeap->GetCPUDescriptorHandleForHeapStart());
    handle.Offset(heapIndex, cbvUavDescriptorSize);

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{
        .BufferLocation = cbAddress,
        .SizeInBytes = passCBByteSize,
    };
    device->CreateConstantBufferView(&cbvDesc, handle);
  }
}

void PBRRenderer::CreatePSOs() {
  D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc{
      .pRootSignature = RootSignature.Get(),
      .VS =
          {
              .pShaderBytecode = Shaders["standardVS"]->GetBufferPointer(),
              .BytecodeLength = Shaders["standardVS"]->GetBufferSize(),
          },
      .PS =
          {
              .pShaderBytecode = Shaders["opaquePS"]->GetBufferPointer(),
              .BytecodeLength = Shaders["opaquePS"]->GetBufferSize(),

          },
      .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
      .SampleMask = UINT_MAX,
      .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
      .DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
      .InputLayout = {.pInputElementDescs = InputLayout.data(),
                      .NumElements = (UINT)InputLayout.size()},
      .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
      .NumRenderTargets = 1,
      .RTVFormats = {backBufferFormat},
      .DSVFormat = depthStencilFormat,
      .SampleDesc =
          DXGI_SAMPLE_DESC{.Count = msaaState ? 4U : 1U,
                           .Quality = msaaState ? (msaaQuality - 1) : 0},

  };

  ThrowIfFailed(device->CreateGraphicsPipelineState(
      &opaquePsoDesc, IID_PPV_ARGS(&PSOs["opaque"])));

  D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = opaquePsoDesc;
  opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
  ThrowIfFailed(device->CreateGraphicsPipelineState(
      &opaqueWireframePsoDesc, IID_PPV_ARGS(&PSOs["opaque_wireframe"])));
}

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

void PBRRenderer::UpdateCamera(const GameTimer &timer) {
  EyePos.x = Radius * sinf(Phi) * cosf(Theta);
  EyePos.z = Radius * sinf(Phi) * sinf(Theta);
  EyePos.y = Radius * cosf(Phi);

  // Build the view matrix.
  XMVECTOR pos = XMVectorSet(EyePos.x, EyePos.y, EyePos.z, 1.0f);
  XMVECTOR target = XMVectorZero();
  XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

  XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
  XMStoreFloat4x4(&ViewMatrix, view);
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
