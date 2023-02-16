//
// Created by arrayJY on 2023/02/13.
//

#pragma once

#include "../dx_utils.h"
#include "../math_helper.h"
#include "../stdafx.h"
#include "pbr_renderer.h"


struct RenderItem {
  DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();

  int NumberFramesDirty = PBRRenderer::GetFrameResourceCount();
  UINT ObjectCBIndex = -1;

  MeshGeometry *Geometry = nullptr;

  D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

  UINT IndexCount = 0;
  UINT StartIndexLocation = 0;
  UINT BaseVertexLocation = 0;
};