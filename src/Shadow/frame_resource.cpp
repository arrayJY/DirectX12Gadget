#include "frame_resource.h"

FrameResource::FrameResource(ID3D12Device *device, UINT passCount,
                             UINT objectCount, UINT materialCount) {
  ThrowIfFailed(device->CreateCommandAllocator(
      D3D12_COMMAND_LIST_TYPE_DIRECT,
      IID_PPV_ARGS(CommandAllocator.GetAddressOf())));
  PassConstantsBuffer =
      std::make_unique<UploadBuffer<PassConstants>>(device, passCount);
  ObjectConstantsBuffer =
      std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCount);
  MaterialConstantsBuffer =
      std::make_unique<UploadBuffer<MaterialConstants>>(device, materialCount);
}
