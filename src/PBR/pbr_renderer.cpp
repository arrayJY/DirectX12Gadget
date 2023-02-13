//
// Created by arrayJY on 2023/02/14.
//

#include "pbr_renderer.h"

void PBRRenderer::Update(const GameTimer &timer) {
  CurrentFrameResourceIndex =
      (CurrentFrameResourceIndex + 1) % FrameResourceCount;
  CurrentFrameResource = FrameResources[CurrentFrameResourceIndex].get();

  if (CurrentFrameResource->Fence != 0 &&
      fence->GetCompletedValue() < CurrentFrameResource->Fence) {
    WaitForFence(CurrentFrameResource->Fence);
  }
}
