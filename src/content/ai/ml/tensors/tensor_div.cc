#include "tensor_div.h"

namespace Engine::ML::Tensors {
Tensor TensorDiv(const Tensor& a, const Tensor& b) { Tensor r(a); r /= b; return r; }
void   TensorDivInPlace(Tensor& a, const Tensor& b) { a /= b; }
}  // namespace Engine::ML::Tensors
