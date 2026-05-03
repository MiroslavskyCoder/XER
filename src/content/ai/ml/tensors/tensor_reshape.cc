#include "tensor_reshape.h"

namespace Engine::ML::Tensors {
Tensor TensorReshape(const Tensor& t, const std::vector<size_t>& new_shape) {
    return t.Reshape(new_shape);
}
}  // namespace Engine::ML::Tensors
