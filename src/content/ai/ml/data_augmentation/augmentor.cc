#include "augmentor.h"

namespace Engine::MLData::Augmentation {

void AugmentationPipeline::AddAugmentor(std::shared_ptr<Augmentor> augmentor) {
    if (augmentor) {
        augmentors_.push_back(augmentor);
    }
}

std::shared_ptr<Types::Tensor> AugmentationPipeline::Apply(const Types::Tensor& input) {
    auto result = std::make_shared<Types::Tensor>(input.GetShape(), input.GetDataType());
    return result;
}

} // namespace Engine::MLData::Augmentation
