//
// Created by arrayJY on 2023/02/14.
//

#include "../renderer.h"
#include "../stdafx.h"
#include "frame_resource.h"

class PBRRenderer : public Renderer {
public:
  PBRRenderer() : Renderer() { renderer = this; }

  void Update(const GameTimer &timer) override;

private:
  const int FrameResourceCount = 3;
  std::vector<std::unique_ptr<FrameResource>> FrameResources;
  FrameResource *CurrentFrameResource;
  int CurrentFrameResourceIndex = 0;
};