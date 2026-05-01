#pragma once

#include "augmentor.h"

namespace Engine::MLData::Augmentation {

class RotationAugmentor : public Augmentor {
public:
    explicit RotationAugmentor(float max_angle = 45.0f);
    
    std::shared_ptr<Types::Tensor> Augment(const Types::Tensor& input) override;
    std::string GetAugmentationName() const override { return "RotationAugmentor"; }
    
    void SetMaxAngle(float angle) { max_angle_ = angle; }

private:
    float max_angle_;
};

} // namespace Engine::MLData::Augmentation
