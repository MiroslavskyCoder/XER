#pragma once
#include "tensor.h"

namespace Engine::ML::Tensors {
/// Cross product of two 3-element tensors (returns 3-element tensor)
Tensor TensorCross(const Tensor& a, const Tensor& b);
}  // namespace Engine::ML::Tensors
