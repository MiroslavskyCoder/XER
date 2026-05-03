#pragma once
#include "tensor.h"

namespace Engine::ML::Tensors {
/// Remove a dimension of size 1 at axis.  If axis==-1 removes first size-1 dim.
Tensor TensorSqueeze(const Tensor& t, int axis = -1);
}  // namespace Engine::ML::Tensors
