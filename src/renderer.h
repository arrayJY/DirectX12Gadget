//
// Created by arrayJY on 2023/02/02.
//

#ifndef DX12DEMO_RENDERER_H
#define DX12DEMO_RENDERER_H

#include "stdafx.h"

class Renderer {
public:
  // function declarations
  bool InitDirectX(); // initializes direct3d 12

  void Update(); // update the game logic

  void UpdatePipeline(); // update the direct3d pipeline (update command lists)

  void Render(); // execute the command list

  void Cleanup(); // release com ojects and clean up memory

  void WaitForPreviousFrame(); // wait until gpu is finished with command list
private:
  // direct3d stuff
  static constexpr int frameBufferCount =
      3; // number of buffers we want, 2 for double
         // buffering, 3 for tripple buffering

  ID3D12Device *device; // direct3d device

  IDXGISwapChain3 *swapChain; // swapchain used to switch between render targets

  ID3D12CommandQueue *commandQueue; // container for command lists

  ID3D12DescriptorHeap *rtvDescriptorHeap; // a descriptor heap to hold
                                           // resources like the render targets

  ID3D12Resource *renderTargets[frameBufferCount]; // number of render targets
                                                   // equal to buffer count

  ID3D12CommandAllocator
      *commandAllocator[frameBufferCount]; // we want enough allocators for each
                                           // buffer * number of threads (we
                                           // only have one thread)

  ID3D12GraphicsCommandList
      *commandList; // a command list we can record commands into, then execute
                    // them to render the frame

  ID3D12Fence
      *fence[frameBufferCount]; // an object that is locked while our command
                                // list is being executed by the gpu. We need as
                                // many
                                // as we have allocators (more if we want to
                                // know when the gpu is finished with an asset)

  HANDLE
  fenceEvent; // a handle to an event when our fence is unlocked by the gpu

  UINT64 fenceValue[frameBufferCount]; // this value is incremented each frame.
                                       // each fence will have its own value

  int frameIndex; // current rtv we are on

  int rtvDescriptorSize; // size of the rtv descriptor on the device (all front
                         // and back buffers will be the same size)
};

#endif