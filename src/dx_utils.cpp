//
// Created by arrayJY on 2023/02/08.
//

#include "dx_utils.h"

int DXUtils::CalcConstantBufferSize(int byteSize) {
  return (byteSize + 0xFF) & (~0xFF);
}

ComPtr<ID3DBlob> DXUtils::CompileShader(const std::wstring &fileName,
                                        const D3D_SHADER_MACRO *defines,
                                        const std::string &entryPoint,
                                        const std::string &target) {
  UINT compileFlags = 0;
#if DEBUG
  compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

  ComPtr<ID3DBlob> byteCode = nullptr;
  ComPtr<ID3DBlob> errors;

  HRESULT hr = D3DCompileFromFile(
      fileName.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
      entryPoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

  if (errors != nullptr) {
    OutputDebugString((char *)errors->GetBufferPointer());
  }

  ThrowIfFailed(hr);
  return byteCode;
}

D3D12_VERTEX_BUFFER_VIEW MeshGeometry::VertexBufferView() const {
  return D3D12_VERTEX_BUFFER_VIEW{
      .BufferLocation = VertexGPUBuffer->GetGPUVirtualAddress(),
      .SizeInBytes = VertexBufferByteSize,
      .StrideInBytes = VertexByteStride,
  };
}

D3D12_INDEX_BUFFER_VIEW MeshGeometry::IndexBufferView() const {
  return {
    .BufferLocation = IndexGPUBuffer->GetGPUVirtualAddress(),
    .SizeInBytes = IndexBufferByteSize,
    .Format = IndexFormat,
  };
}

void MeshGeometry::DisposeUploaders() {
  VertexBufferUploader.Reset();
  IndexBufferUploader.Reset();
}
