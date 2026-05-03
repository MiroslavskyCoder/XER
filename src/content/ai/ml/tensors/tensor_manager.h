#pragma once

#include "tensor.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace Engine::ML::Tensors {

/// Registry for named tensors (activations, weights, etc.)
class TensorManager {
public:
    void Register(const std::string& name, std::shared_ptr<Tensor> tensor);
    std::shared_ptr<Tensor> Get(const std::string& name) const;
    bool Has(const std::string& name) const;
    void Remove(const std::string& name);
    void Clear();

    size_t Count() const { return registry_.size(); }

private:
    std::unordered_map<std::string, std::shared_ptr<Tensor>> registry_;
};

}  // namespace Engine::ML::Tensors
