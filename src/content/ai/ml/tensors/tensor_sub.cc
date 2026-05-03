#include "tensor_sub.h"

namespace Engine::ML::Tensors {
Tensor TensorSub(const Tensor& a, const Tensor& b) { Tensor r(a); r -= b; return r; }
void TensorSubInPlace(Tensor& a, const Tensor& b) { a -= b; }
}  // namespace Engine::ML::Tensors
