#include "augmentor.h"

namespace Engine::ML::DataAugmentation {

void AugmentationPipeline::AddAugmentor(std::shared_ptr<Augmentor> augmentor) {
    if (augmentor) {
        augmentors_.push_back(augmentor);
    }
}

Engine::MLData::Types::Tensor AugmentationPipeline::Apply(const Engine::MLData::Types::Tensor& input) {
    auto result = Engine::MLData::Types::Tensor(input.GetShape(), input.GetDataType());
    return result;
}

} // namespace Engine::ML::DataAugmentation
