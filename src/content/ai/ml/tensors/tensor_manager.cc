#include "tensor_manager.h"

namespace Engine::ML::Tensors {

void TensorManager::Register(const std::string& name, std::shared_ptr<Tensor> tensor) {
    registry_[name] = std::move(tensor);
}

std::shared_ptr<Tensor> TensorManager::Get(const std::string& name) const {
    auto it = registry_.find(name);
    return (it != registry_.end()) ? it->second : nullptr;
}

bool TensorManager::Has(const std::string& name) const {
    return registry_.count(name) > 0;
}

void TensorManager::Remove(const std::string& name) {
    registry_.erase(name);
}

void TensorManager::Clear() {
    registry_.clear();
}

}  // namespace Engine::ML::Tensors
