//
// Created by arrayJY on 2023/02/09.
//
#pragma once

#include "stdafx.h"

class MathHelper {
public:
  static DirectX::XMFLOAT4X4 Identity4x4() {
    return DirectX::XMFLOAT4X4{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
  }

  static constexpr auto PI = 3.1415926;



};