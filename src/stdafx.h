//
// Created by arrayJY on 2023/02/02.
//

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers.
#endif

#include "d3dx12.h"
#include <DirectXMath.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>
#include <windows.h>

#define SAFE_RELEASE(p)                                                        \
  {                                                                            \
    if ((p)) {                                                                 \
      (p)->Release();                                                          \
      (p) = 0;                                                                 \
    }                                                                          \
  }

class DxException : public std::exception {
public:
  DxException() = default;
  DxException(HRESULT hr, const std::wstring &functionName,
              const std::wstring &filename, int lineNumber) {}
  const char *what() const override {
    static char s[1024] = {};
    sprintf_s(s, "%s: %s: line %d: Error %8d \n", FileName.c_str(),
              FunctionName.c_str(), LineNumber,
              static_cast<unsigned int>(ErrorCode));
    return s;
  }

private:
  HRESULT ErrorCode = S_OK;
  std::wstring FunctionName;
  std::wstring FileName;
  int LineNumber = -1;
};

inline std::wstring AnsiToWString(const std::string &str) {
  WCHAR buffer[512];
  MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
  return std::wstring(buffer);
}

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                                       \
  {                                                                            \
    HRESULT __hr = (x);                                                        \
    std::wstring wfn = AnsiToWString(__FILE__);                                \
    if (FAILED(__hr)) {                                                        \
      throw DxException(__hr, L#x, wfn, __LINE__);                             \
    }                                                                          \
  }

#endif // !ThrowIfFailed