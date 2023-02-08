//
// Created by arrayJY on 2023/02/08.
//

#pragma once
#include "stdafx.h"

class DXUtils {
public:
  static int CalcConstantBufferSize(int byteSize);
  static ComPtr<ID3DBlob> CompileShader(const std::wstring &fileName,
                                        const D3D_SHADER_MACRO *defines,
                                        const std::string &entryPoint,
                                        const std::string &target);
};

struct SubmeshGeometry {
  UINT IndexCount = 0;
  UINT StartIndexLocation = 0;
  INT BaseVertexLocation = 0;
  DirectX::BoundingBox Bounds;
};

struct MeshGeometry {
  std::string Name;

  ComPtr<ID3DBlob> VertexCPUBuffer = nullptr;
  ComPtr<ID3DBlob> IndexCPUBuffer = nullptr;
  ComPtr<ID3D12Resource> VertexGPUBuffer = nullptr;
  ComPtr<ID3D12Resource> IndexGPUBuffer = nullptr;

  ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
  ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

  UINT VertexByteStride = 0;
  UINT VertexBufferByteSize = 0;
  DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
  UINT IndexBufferByteSize = 0;

  std::unordered_map<std::string, SubmeshGeometry> DrawArgs;

  D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const;
  D3D12_INDEX_BUFFER_VIEW IndexBufferView() const;
  void DisposeUploaders();
};