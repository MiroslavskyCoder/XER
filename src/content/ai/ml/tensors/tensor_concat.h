#pragma once
#include "tensor.h"
#include <vector>

namespace Engine::ML::Tensors {
/// Concatenate tensors along a given axis
Tensor TensorConcat(const std::vector<Tensor>& tensors, int axis = 0);
}  // namespace Engine::ML::Tensors
