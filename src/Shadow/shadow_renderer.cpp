//
// Created by arrayJY on 2023/03/26.
//

#include "shadow_renderer.h"
#include "../geometry_generator.h"
#include <DDSTextureLoader.h>
#include <ResourceUploadBatch.h>

using namespace DirectX;

void ShadowRenderer::InitDirectX(const InitInfo &initInfo) {
  Renderer::InitDirectX(initInfo);

  ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));

  shadowMap = std::make_unique<ShadowMap>(device.Get(), 2048, 2048);

  LoadTextures();
  CreateRootSignature();
  CreateDescriptorHeaps();
  CreateShadersAndInputLayout();
  CreateShapeGeometry();
  CreateFrameResources();
  CreateMaterials();
  CreateRenderItems();
  CreatePSOs();

  ThrowIfFailed(commandList->Close());
  ID3D12CommandList *cmdsLists[] = {commandList.Get()};
  commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

  FlushCommandQueue();
}
void ShadowRenderer::OnResize(UINT width, UINT height) {
  Renderer::OnResize(width, height);
  camera.SetLens(0.25f * MathHelper::PI, AspectRatio(), 1.0f, 1000.0f);
}

void ShadowRenderer::Draw(const GameTimer &timer) {
  auto cmdListAllocater = CurrentFrameResource->CommandAllocator;

  ThrowIfFailed(cmdListAllocater->Reset());

  ID3D12DescriptorHeap *descriptorHeaps[] = {srvDescriptorHeap.Get()};
  commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

  commandList->SetGraphicsRootSignature(RootSignature.Get());

  auto matBuffer = CurrentFrameResource->MaterialConstantsBuffer->Resource();
  commandList->SetGraphicsRootShaderResourceView(
      2, matBuffer->GetGPUVirtualAddress());

  commandList->SetGraphicsRootDescriptorTable(3, NullSrv);

  commandList->SetGraphicsRootDescriptorTable(
      4, srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

  DrawSceneToShadowMap();

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

  auto currentBackBuffer = CurrentBackBufferView();
  auto depthStencilView = DepthStencilView();
  commandList->OMSetRenderTargets(1, &currentBackBuffer, true,
                                  &depthStencilView);

}

void ShadowRenderer::Update(const GameTimer &timer) {}

void ShadowRenderer::LoadTextures() {
  // TODO
  std::vector<std::pair<std::string, std::wstring>> textures{

  };

  DirectX::ResourceUploadBatch upload(device.Get());

  for (auto &tex : textures) {
    std::wstring path = TEXTURE_DIR + tex.second;
    auto texMap = std::make_unique<Texture>();
    texMap->Name = tex.first;
    texMap->FileName = path;
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile(
        device.Get(), upload, texMap->FileName.c_str(),
        texMap->Resource.GetAddressOf()))
  }

  auto finish = upload.End(commandQueue.Get());
  finish.wait();
}

void ShadowRenderer::CreateRootSignature() {
  CD3DX12_DESCRIPTOR_RANGE texTable[2];
  texTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0);
  texTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 2);

  CD3DX12_ROOT_PARAMETER slotRootParameter[5];
  slotRootParameter[0].InitAsConstantBufferView(0);
  slotRootParameter[1].InitAsConstantBufferView(1);
  slotRootParameter[2].InitAsShaderResourceView(0, 1);
  slotRootParameter[3].InitAsDescriptorTable(1, &texTable[0],
                                             D3D12_SHADER_VISIBILITY_PIXEL);
  slotRootParameter[4].InitAsDescriptorTable(1, &texTable[1],
                                             D3D12_SHADER_VISIBILITY_PIXEL);

  auto staticSamplers = GetStaticSamplers();

  CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
      5, slotRootParameter, static_cast<UINT>(staticSamplers.size()),
      staticSamplers.data(),
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

void ShadowRenderer::CreateDescriptorHeaps() {
  // SRV
  auto srvHeapDesc = D3D12_DESCRIPTOR_HEAP_DESC{
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = 14,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
  };
  ThrowIfFailed(device->CreateDescriptorHeap(
      &srvHeapDesc, IID_PPV_ARGS(srvDescriptorHeap.GetAddressOf())));

  CD3DX12_CPU_DESCRIPTOR_HANDLE descriptorHandle(
      srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
  std::vector<ComPtr<ID3D12Resource>> texturesList; // TODO: fill texture list

  auto srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC{
      .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
      .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
      .Texture2D =
          D3D12_TEX2D_SRV{
              .MostDetailedMip = 0,
              .ResourceMinLODClamp = 0.0f,
          },
  };

  for (auto &i : texturesList) {
    srvDesc.Format = i->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = i->GetDesc().MipLevels;
    device->CreateShaderResourceView(i.Get(), &srvDesc, descriptorHandle);
    descriptorHandle.Offset(1, cbvUavDescriptorSize);
  }

  ShadowMapHeapIndex = texturesList.size();
  NullTextureSrvIndex = ShadowMapHeapIndex + 1;

  auto srvCPUStart = srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
  auto srvGPUStart = srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
  auto dsvCPUStart = dsvHeap->GetCPUDescriptorHandleForHeapStart();

  auto nullSrv = CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCPUStart, NullTextureSrvIndex,
                                               cbvUavDescriptorSize);
  NullSrv = CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGPUStart, NullTextureSrvIndex,
                                          cbvUavDescriptorSize);

  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  srvDesc.Texture2D = D3D12_TEX2D_SRV{
      .MostDetailedMip = 0,
      .MipLevels = 1,
      .PlaneSlice = 0,
      .ResourceMinLODClamp = 0.0f,
  };

  device->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);

  shadowMap->BuildDescriptors(
      CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCPUStart, ShadowMapHeapIndex,
                                    cbvUavDescriptorSize),
      CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGPUStart, ShadowMapHeapIndex,
                                    cbvUavDescriptorSize),
      CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvCPUStart, 1, dsvDescriptorSize));
}

void ShadowRenderer::CreateShadersAndInputLayout() {
  constexpr D3D_SHADER_MACRO alphaTestDefines[] = {"ALPHA_TEST", "1", nullptr,
                                                   nullptr};

  Shaders["mainVS"] =
      DXUtils::CompileShader(SHADER_DIR L"/main.hlsl", nullptr, "VS", "vs_5_1");
  Shaders["mainOpaquePS"] =
      DXUtils::CompileShader(SHADER_DIR L"/main.hlsl", nullptr, "PS", "ps_5_1");
  Shaders["shadowVS"] = DXUtils::CompileShader(SHADER_DIR L"/shadow.hlsl",
                                               nullptr, "VS", "vs_5_1");
  Shaders["shadowOpaquePS"] = DXUtils::CompileShader(SHADER_DIR L"/shadow.hlsl",
                                                     nullptr, "PS", "ps_5_1");
  Shaders["shadowAlphaPS"] = DXUtils::CompileShader(SHADER_DIR L"/shadow.hlsl",
                                                    nullptr, "PS", "ps_5_1");

  inputLayout = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"TANGENT", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 32,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  };
}

void ShadowRenderer::CreateShapeGeometry() {
  GeometryGenerator geoGen;
  GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3);
  GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
  GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
  GeometryGenerator::MeshData cylinder =
      geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);
  GeometryGenerator::MeshData quad =
      geoGen.CreateQuad(0.0f, 0.0f, 1.0f, 1.0f, 0.0f);

  //
  // We are concatenating all the geometry into one big vertex/index buffer.  So
  // define the regions in the buffer each submesh covers.
  //

  // Cache the vertex offsets to each object in the concatenated vertex buffer.
  UINT boxVertexOffset = 0;
  UINT gridVertexOffset = (UINT)box.Vertices.size();
  UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
  UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();
  UINT quadVertexOffset = cylinderVertexOffset + (UINT)cylinder.Vertices.size();

  // Cache the starting index for each object in the concatenated index buffer.
  UINT boxIndexOffset = 0;
  UINT gridIndexOffset = (UINT)box.Indices32.size();
  UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
  UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();
  UINT quadIndexOffset = cylinderIndexOffset + (UINT)cylinder.Indices32.size();

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

  SubmeshGeometry quadSubmesh;
  quadSubmesh.IndexCount = (UINT)quad.Indices32.size();
  quadSubmesh.StartIndexLocation = quadIndexOffset;
  quadSubmesh.BaseVertexLocation = quadVertexOffset;

  //
  // Extract the vertex elements we are interested in and pack the
  // vertices of all the meshes into one vertex buffer.
  //

  auto totalVertexCount = box.Vertices.size() + grid.Vertices.size() +
                          sphere.Vertices.size() + cylinder.Vertices.size() +
                          quad.Vertices.size();

  std::vector<Vertex> vertices(totalVertexCount);

  UINT k = 0;
  for (size_t i = 0; i < box.Vertices.size(); ++i, ++k) {
    vertices[k].Pos = box.Vertices[i].Position;
    vertices[k].Normal = box.Vertices[i].Normal;
    vertices[k].TexC = box.Vertices[i].TexC;
    vertices[k].TangentU = box.Vertices[i].TangentU;
  }

  for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k) {
    vertices[k].Pos = grid.Vertices[i].Position;
    vertices[k].Normal = grid.Vertices[i].Normal;
    vertices[k].TexC = grid.Vertices[i].TexC;
    vertices[k].TangentU = grid.Vertices[i].TangentU;
  }

  for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k) {
    vertices[k].Pos = sphere.Vertices[i].Position;
    vertices[k].Normal = sphere.Vertices[i].Normal;
    vertices[k].TexC = sphere.Vertices[i].TexC;
    vertices[k].TangentU = sphere.Vertices[i].TangentU;
  }

  for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k) {
    vertices[k].Pos = cylinder.Vertices[i].Position;
    vertices[k].Normal = cylinder.Vertices[i].Normal;
    vertices[k].TexC = cylinder.Vertices[i].TexC;
    vertices[k].TangentU = cylinder.Vertices[i].TangentU;
  }

  for (int i = 0; i < quad.Vertices.size(); ++i, ++k) {
    vertices[k].Pos = quad.Vertices[i].Position;
    vertices[k].Normal = quad.Vertices[i].Normal;
    vertices[k].TexC = quad.Vertices[i].TexC;
    vertices[k].TangentU = quad.Vertices[i].TangentU;
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
  indices.insert(indices.end(), std::begin(quad.GetIndices16()),
                 std::end(quad.GetIndices16()));

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
  geo->DrawArgs["quad"] = quadSubmesh;

  Geometries[geo->Name] = std::move(geo);
}

void ShadowRenderer::CreateMaterials() {
  auto bricks0 = std::make_unique<Material>();
  bricks0->Name = "bricks0";
  bricks0->MatCBIndex = 0;
  bricks0->DiffuseSrvHeapIndex = 0;
  bricks0->NormalSrvHeapIndex = 1;
  bricks0->Albedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
  bricks0->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
  bricks0->Roughness = 0.3f;

  auto tile0 = std::make_unique<Material>();
  tile0->Name = "tile0";
  tile0->MatCBIndex = 1;
  tile0->DiffuseSrvHeapIndex = 2;
  tile0->NormalSrvHeapIndex = 3;
  tile0->Albedo = XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f);
  tile0->FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
  tile0->Roughness = 0.1f;

  auto mirror0 = std::make_unique<Material>();
  mirror0->Name = "mirror0";
  mirror0->MatCBIndex = 2;
  mirror0->DiffuseSrvHeapIndex = 4;
  mirror0->NormalSrvHeapIndex = 5;
  mirror0->Albedo = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
  mirror0->FresnelR0 = XMFLOAT3(0.98f, 0.97f, 0.95f);
  mirror0->Roughness = 0.1f;

  auto skullMat = std::make_unique<Material>();
  skullMat->Name = "skullMat";
  skullMat->MatCBIndex = 3;
  skullMat->DiffuseSrvHeapIndex = 4;
  skullMat->NormalSrvHeapIndex = 5;
  skullMat->Albedo = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
  skullMat->FresnelR0 = XMFLOAT3(0.6f, 0.6f, 0.6f);
  skullMat->Roughness = 0.2f;

  auto sky = std::make_unique<Material>();
  sky->Name = "sky";
  sky->MatCBIndex = 4;
  sky->DiffuseSrvHeapIndex = 6;
  sky->NormalSrvHeapIndex = 7;
  sky->Albedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
  sky->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
  sky->Roughness = 1.0f;

  Materials["bricks0"] = std::move(bricks0);
  Materials["tile0"] = std::move(tile0);
  Materials["mirror0"] = std::move(mirror0);
  Materials["skullMat"] = std::move(skullMat);
  Materials["sky"] = std::move(sky);
}

void ShadowRenderer::CreateRenderItems() {}

void ShadowRenderer::CreateFrameResources() {
  for (int i = 0; i < FrameResourceCount; ++i) {
    FrameResources.push_back(std::make_unique<FrameResource>(
        device.Get(), 1, (UINT)AllRenderItems.size(), (UINT)Materials.size()));
  }
}

void ShadowRenderer::CreatePSOs() {
  // Main pass
  auto mainPSODesc = D3D12_GRAPHICS_PIPELINE_STATE_DESC{
      .pRootSignature = RootSignature.Get(),
      .VS = CD3DX12_SHADER_BYTECODE(Shaders["mainVS"].Get()),
      .PS = CD3DX12_SHADER_BYTECODE(Shaders["mainOpaquePS"].Get()),
      .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
      .SampleMask = UINT_MAX,
      .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
      .DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
      .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
      .NumRenderTargets = 1,
      .RTVFormats = {backBufferFormat},
      .DSVFormat = depthStencilFormat,
      .SampleDesc =
          {
              .Count = msaaState ? 4U : 1U,
              .Quality = msaaState ? (msaaQuality - 1) : 0U,
          },
  };
  ThrowIfFailed(device->CreateGraphicsPipelineState(
      &mainPSODesc, IID_PPV_ARGS(&PSOs["opaque"])));

  // Shadow map pass
  auto samplePsoDesc = D3D12_GRAPHICS_PIPELINE_STATE_DESC{
      .pRootSignature = RootSignature.Get(),
      .VS = CD3DX12_SHADER_BYTECODE(Shaders["shadowVS"].Get()),
      .PS = CD3DX12_SHADER_BYTECODE(Shaders["shadowOpaquePS"].Get()),
      .RasterizerState =
          {
              .DepthBias = 100000,
              .DepthBiasClamp = 1.0f,
              .SlopeScaledDepthBias = 1.0f,
          },
      .NumRenderTargets = 0,
      .RTVFormats = {DXGI_FORMAT_UNKNOWN},
  };
  ThrowIfFailed(device->CreateGraphicsPipelineState(
      &samplePsoDesc, IID_PPV_ARGS(&PSOs["shadowOpaque"])));
}

void ShadowRenderer::DrawRenderItems(
    ID3D12GraphicsCommandList *cmdList,
    const std::vector<RenderItem *> &renderItems) {
  UINT objCBByteSize = DXUtils::CalcConstantBufferSize(sizeof(ObjectConstants));

  auto objectCB = CurrentFrameResource->ObjectConstantsBuffer->Resource();

  for (auto &item : renderItems) {
    auto vertexBufferView = item->Geo->VertexBufferView();
    auto indexBufferView = item->Geo->IndexBufferView();
    cmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
    cmdList->IASetIndexBuffer(&indexBufferView);
    cmdList->IASetPrimitiveTopology(item->PrimitiveType);
    D3D12_GPU_VIRTUAL_ADDRESS objCBAddress =
        objectCB->GetGPUVirtualAddress() + item->ObjCBIndex * objCBByteSize;

    cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

    cmdList->DrawIndexedInstanced(item->IndexCount, 1, item->StartIndexLocation,
                                  item->BaseVertexLocation, 0);
  }
}

void ShadowRenderer::DrawSceneToShadowMap() {
  auto shadowViewport = shadowMap->Viewport();
  auto shadowScissorRect = shadowMap->ScissorRect();
  commandList->RSSetViewports(1, &shadowViewport);
  commandList->RSSetScissorRects(1, &shadowScissorRect);

  // Change to DEPTH_WRITE
  auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
      shadowMap->Resource(), D3D12_RESOURCE_STATE_GENERIC_READ,
      D3D12_RESOURCE_STATE_DEPTH_WRITE);
  commandList->ResourceBarrier(1, &barrier);

  UINT passCBByteSize = DXUtils::CalcConstantBufferSize(sizeof(PassConstants));
  auto shadowMapDsv = shadowMap->Dsv();
  commandList->ClearDepthStencilView(
      shadowMapDsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0,
      0, nullptr);

  // Set null render target
  commandList->OMSetRenderTargets(0, nullptr, false, &shadowMapDsv);

  auto passCB = CurrentFrameResource->PassConstantsBuffer->Resource();
  D3D12_GPU_VIRTUAL_ADDRESS passCBAddress =
      passCB->GetGPUVirtualAddress() + 1 * passCBByteSize;
  commandList->SetGraphicsRootConstantBufferView(1, passCBAddress);

  commandList->SetPipelineState(PSOs["shadowOpaque"].Get());

  DrawRenderItems(commandList.Get(), LayerItems[(int)RenderLayer::Opaque]);

  // Change to GENERIC_READ
  barrier = CD3DX12_RESOURCE_BARRIER::Transition(
      shadowMap->Resource(), D3D12_RESOURCE_STATE_DEPTH_WRITE,
      D3D12_RESOURCE_STATE_GENERIC_READ);
  commandList->ResourceBarrier(1, &barrier);
}
