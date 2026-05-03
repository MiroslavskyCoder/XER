#include "tensor_transpose.h"

namespace Engine::ML::Tensors {
Tensor TensorTranspose(const Tensor& t) { return t.Transpose(); }
}  // namespace Engine::ML::Tensors
