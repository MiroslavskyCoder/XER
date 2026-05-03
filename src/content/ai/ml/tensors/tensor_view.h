#pragma once
#include "tensor.h"
#include <vector>

namespace Engine::ML::Tensors {
/// Non-owning view: reinterpret an existing Tensor with a different shape
/// (returns a reshaped clone; for true views use Reshape)
Tensor TensorView(const Tensor& t, const std::vector<size_t>& new_shape);
}  // namespace Engine::ML::Tensors
