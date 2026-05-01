#include "tensor.h"
#include <algorithm>

namespace Engine::MLData::Types {

static size_t GetTypeSize(DataType dtype) {
    switch (dtype) {
        case DataType::FLOAT32: return 4;
        case DataType::FLOAT64: return 8;
        case DataType::INT32: return 4;
        case DataType::INT64: return 8;
        case DataType::UINT32: return 4;
        case DataType::UINT8: return 1;
        default: return 0;
    }
}

Tensor::Tensor(const std::vector<uint32_t>& shape, DataType dtype)
    : shape_(shape), data_type_(dtype), element_count_(1) {
    
    for (auto s : shape_) {
        element_count_ *= s;
    }
    
    size_t type_size = GetTypeSize(dtype);
    memory_size_ = element_count_ * type_size;
    data_ = malloc(memory_size_);
}

Tensor::~Tensor() {
    if (data_) {
        free(data_);
        data_ = nullptr;
    }
}

} // namespace Engine::MLData::Types
