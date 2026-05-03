#include "tensor_matmul.h"

namespace Engine::ML::Tensors {
Tensor TensorMatMul(const Tensor& a, const Tensor& b) { return a.MatMul(b); }
}  // namespace Engine::ML::Tensors
