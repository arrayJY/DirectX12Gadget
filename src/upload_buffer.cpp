//
// Created by arrayJY on 2023/02/07.
//

#include "upload_buffer.h"


template <typename T>
void UploadBuffer<T>::CopyData(int elementIndex, const T &data) {
  std::memcpy(&mappedData[elementIndex * elementSize], &data, sizeof(T));
}
