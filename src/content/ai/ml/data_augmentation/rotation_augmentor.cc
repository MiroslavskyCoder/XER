#include "rotation_augmentor.h"

namespace Engine::MLData::Augmentation {

RotationAugmentor::RotationAugmentor(float max_angle)
    : max_angle_(max_angle) {
}

std::shared_ptr<Types::Tensor> RotationAugmentor::Augment(const Types::Tensor& input) {
    auto result = std::make_shared<Types::Tensor>(input.GetShape(), input.GetDataType());
    return result;
}

} // namespace Engine::MLData::Augmentation
