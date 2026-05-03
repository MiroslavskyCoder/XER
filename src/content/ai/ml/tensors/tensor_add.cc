#include "tensor_add.h"

namespace Engine::ML::Tensors {

Tensor TensorAdd(const Tensor& a, const Tensor& b) {
    Tensor r(a);
    r += b;
    return r;
}

void TensorAddInPlace(Tensor& a, const Tensor& b) {
    a += b;
}

}  // namespace Engine::ML::Tensors
