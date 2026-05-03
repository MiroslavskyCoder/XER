#include "tensor_transpose_matmul.h"

namespace Engine::ML::Tensors {
Tensor TensorTransposeMatMul(const Tensor& a, const Tensor& b) {
    return a.Transpose().MatMul(b);
}
}  // namespace Engine::ML::Tensors
