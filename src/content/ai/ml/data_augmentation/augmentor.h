#pragma once

#include "../data_types/tensor.h"
#include <random>

namespace Engine::MLData::Augmentation {

class Augmentor {
public:
    virtual ~Augmentor() = default;
    
    virtual std::shared_ptr<Types::Tensor> Augment(const Types::Tensor& input) = 0;
    virtual std::string GetAugmentationName() const = 0;
};

class AugmentationPipeline {
public:
    void AddAugmentor(std::shared_ptr<Augmentor> augmentor);
    std::shared_ptr<Types::Tensor> Apply(const Types::Tensor& input);
    
    size_t GetAugmentorCount() const { return augmentors_.size(); }

private:
    std::vector<std::shared_ptr<Augmentor>> augmentors_;
};

} // namespace Engine::MLData::Augmentation
