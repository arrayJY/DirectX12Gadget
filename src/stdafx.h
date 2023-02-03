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

#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }