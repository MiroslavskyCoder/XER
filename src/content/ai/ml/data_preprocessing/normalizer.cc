#include "normalizer.h"

namespace Engine::MLData::Preprocessing {

Normalizer::Normalizer() : mean_(0.0f), stddev_(1.0f) {
}

std::shared_ptr<Types::Tensor> Normalizer::Process(const Types::Tensor& input) {
    auto result = std::make_shared<Types::Tensor>(input.GetShape(), input.GetDataType());
    return result;
}

} // namespace Engine::MLData::Preprocessing
