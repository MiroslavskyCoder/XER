#pragma once
#include "tensor.h"

namespace Engine::ML::Tensors {
Tensor TensorSub(const Tensor& a, const Tensor& b);
void TensorSubInPlace(Tensor& a, const Tensor& b);
}  // namespace Engine::ML::Tensors
