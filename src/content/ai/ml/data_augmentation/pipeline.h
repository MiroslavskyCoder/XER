#pragma once

#include "../tensors/tensor.h"
#include <vector>
#include <memory>

namespace Engine::ML::DataAugmentation {

// Forward declare augmentors
class Augmentor;

/// @brief Data augmentation pipeline
/// 
/// Chains multiple augmentors and applies them sequentially to input
/// during training. Can be configured differently for train/eval modes.
class AugmentationPipeline {
public:
    /// Create empty pipeline
    AugmentationPipeline() = default;
    
    /// Add augmentor to pipeline
    /// @param augmentor Augmentor to add (ownership transferred)
    void AddAugmentor(std::unique_ptr<Augmentor> augmentor) {
        augmentors_.push_back(std::move(augmentor));
    }
    
    /// Clear all augmentors
    void Clear() {
        augmentors_.clear();
    }
    
    /// Apply all augmentations in sequence
    /// @param input Input tensor
    /// @return Augmented tensor after applying all augmentors
    Tensors::Tensor Apply(const Tensors::Tensor& input) {
        auto result = input;
        for (auto& aug : augmentors_) {
            if (aug) {
                result = aug->Augment(result);
            }
        }
        return result;
    }
    
    /// Enable/disable entire pipeline
    /// @param enabled Training mode (true) vs inference mode (false)
    void SetEnabled(bool enabled) {
        for (auto& aug : augmentors_) {
            if (aug) {
                aug->SetEnabled(enabled);
            }
        }
    }
    
    /// Get number of augmentors in pipeline
    size_t GetNumAugmentors() const {
        return augmentors_.size();
    }

private:
    std::vector<std::unique_ptr<Augmentor>> augmentors_;
};

} // namespace Engine::ML::DataAugmentation
