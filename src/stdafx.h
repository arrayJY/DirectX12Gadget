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

// Helper class for COM exceptions
class com_exception : public std::exception {
public:
  com_exception(HRESULT hr) : result(hr) {}

  virtual const char *what() const override {
    static char s_str[64] = {};
    sprintf_s(s_str, "Failure with HRESULT of %08X",
              static_cast<unsigned int>(result));
    return s_str;
  }

private:
  HRESULT result;
};

// Helper utility converts D3D API failures into exceptions.
inline void ThrowIfFailed(HRESULT hr) {
  if (FAILED(hr)) {
    throw com_exception(hr);
  }
}