#pragma once
#include "tensor.h"

namespace Engine::ML::Tensors {
Tensor TensorDiv(const Tensor& a, const Tensor& b);
void   TensorDivInPlace(Tensor& a, const Tensor& b);
}  // namespace Engine::ML::Tensors
