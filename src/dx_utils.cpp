//
// Created by arrayJY on 2023/02/08.
//

#include "dx_utils.h"

int DXUtils::CalcConstantBufferSize(int byteSize) {
  return (byteSize + 0xFF) & (~0xFF);
}
