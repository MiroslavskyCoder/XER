#pragma once
#include "tensor.h"
#include <vector>

namespace Engine::ML::Tensors {
/// Extract a row-slice [start, start+length) from a 2D tensor along axis 0
Tensor TensorSlice(const Tensor& t, size_t start, size_t length);
}  // namespace Engine::ML::Tensors
