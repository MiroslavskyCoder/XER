#pragma once
#include "tensor.h"

namespace Engine::ML::Tensors {
/// Transpose of a 2D tensor
Tensor TensorTranspose(const Tensor& t);
}  // namespace Engine::ML::Tensors
