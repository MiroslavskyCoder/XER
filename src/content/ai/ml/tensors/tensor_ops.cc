#include "tensor_ops.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace Engine::ML::Tensors {

bool TensorBroadcast::IsCompatible(const std::vector<size_t>& shape1,
                                    const std::vector<size_t>& shape2) {
    size_t max_dims = std::max(shape1.size(), shape2.size());
    
    for (int i = 1; i <= max_dims; ++i) {
        size_t dim1 = (i <= shape1.size()) ? shape1[shape1.size() - i] : 1;
        size_t dim2 = (i <= shape2.size()) ? shape2[shape2.size() - i] : 1;
        
        if (dim1 != dim2 && dim1 != 1 && dim2 != 1) {
            return false;
        }
    }
    
    return true;
}

std::vector<size_t> TensorBroadcast::GetOutputShape(const std::vector<size_t>& shape1,
                                                     const std::vector<size_t>& shape2) {
    if (!IsCompatible(shape1, shape2)) {
        throw std::invalid_argument("Shapes not broadcastable");
    }
    
    size_t max_dims = std::max(shape1.size(), shape2.size());
    std::vector<size_t> output_shape(max_dims);
    
    for (int i = 1; i <= max_dims; ++i) {
        size_t dim1 = (i <= shape1.size()) ? shape1[shape1.size() - i] : 1;
        size_t dim2 = (i <= shape2.size()) ? shape2[shape2.size() - i] : 1;
        output_shape[max_dims - i] = std::max(dim1, dim2);
    }
    
    return output_shape;
}

Tensor TensorBroadcast::Broadcast(const Tensor& tensor,
                                   const std::vector<size_t>& target_shape) {
    const auto& current_shape = tensor.Shape();
    
    if (current_shape == target_shape) {
        return tensor.Clone();
    }
    
    // Simple broadcast: replicate along dimensions of size 1
    // Full implementation would need proper broadcasting logic
    return tensor.Clone();
}

Tensor TensorSlice::GetRow(const Tensor& tensor, size_t row_idx) {
    const auto& shape = tensor.Shape();
    if (shape.size() != 2) {
        throw std::invalid_argument("GetRow requires 2D tensor");
    }
    
    if (row_idx >= shape[0]) {
        throw std::out_of_range("Row index out of bounds");
    }
    
    // Create row vector
    auto row_data = tensor.Data().row(row_idx);
    return Tensor(row_data, tensor.GetDevice());
}

Tensor TensorSlice::GetColumn(const Tensor& tensor, size_t col_idx) {
    const auto& shape = tensor.Shape();
    if (shape.size() != 2) {
        throw std::invalid_argument("GetColumn requires 2D tensor");
    }
    
    if (col_idx >= shape[1]) {
        throw std::out_of_range("Column index out of bounds");
    }
    
    // Create column vector
    auto col_data = tensor.Data().col(col_idx);
    return Tensor(col_data, tensor.GetDevice());
}

Tensor TensorSlice::GetBlock(const Tensor& tensor,
                              size_t row_start, size_t row_count,
                              size_t col_start, size_t col_count) {
    const auto& shape = tensor.Shape();
    if (shape.size() != 2) {
        throw std::invalid_argument("GetBlock requires 2D tensor");
    }
    
    if (row_start + row_count > shape[0] || col_start + col_count > shape[1]) {
        throw std::out_of_range("Block out of bounds");
    }
    
    auto block_data = tensor.Data().block(row_start, col_start, row_count, col_count);
    return Tensor(block_data, tensor.GetDevice());
}

Tensor TensorConcat::Concatenate(const std::vector<Tensor>& tensors, int axis) {
    if (tensors.empty()) {
        throw std::invalid_argument("Cannot concatenate empty tensor list");
    }
    
    // All tensors must have same number of dimensions and same size in other axes
    const auto& first_shape = tensors[0].Shape();
    
    for (const auto& t : tensors) {
        if (t.Shape().size() != first_shape.size()) {
            throw std::invalid_argument("All tensors must have same rank");
        }
    }
    
    // For 2D concatenation along axis 0 (rows)
    if (axis == 0 && first_shape.size() == 2) {
        size_t total_rows = 0;
        size_t cols = first_shape[1];
        
        for (const auto& t : tensors) {
            if (t.Shape()[1] != cols) {
                throw std::invalid_argument("Mismatch in non-concatenation axis");
            }
            total_rows += t.Shape()[0];
        }
        
        Tensor result({total_rows, cols}, tensors[0].GetDevice());
        size_t row_offset = 0;
        
        for (const auto& t : tensors) {
            size_t rows = t.Shape()[0];
            result.MutableData().block(row_offset, 0, rows, cols) = t.Data();
            row_offset += rows;
        }
        
        return result;
    }
    
    throw std::invalid_argument("Concatenation not implemented for this axis");
}

Tensor TensorConcat::Stack(const std::vector<Tensor>& tensors, int axis) {
    if (tensors.empty()) {
        throw std::invalid_argument("Cannot stack empty tensor list");
    }
    
    // Stack creates new dimension
    // For now, simplified: assume stacking 2D tensors to 3D
    size_t num_tensors = tensors.size();
    const auto& first_shape = tensors[0].Shape();
    
    // Create output shape [num_tensors, rows, cols]
    std::vector<size_t> output_shape = {num_tensors};
    for (size_t dim : first_shape) {
        output_shape.push_back(dim);
    }
    
    // For now, stack as concatenation
    return Concatenate(tensors, 0);
}

} // namespace Engine::ML::Tensors
