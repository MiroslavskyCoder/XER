#include "scaler.h"

namespace Engine::MLData::Preprocessing {

Scaler::Scaler(ScalingMode mode) : mode_(mode) {
}

std::shared_ptr<Types::Tensor> Scaler::Process(const Types::Tensor& input) {
    auto result = std::make_shared<Types::Tensor>(input.GetShape(), input.GetDataType());
    return result;
}

} // namespace Engine::MLData::Preprocessing
