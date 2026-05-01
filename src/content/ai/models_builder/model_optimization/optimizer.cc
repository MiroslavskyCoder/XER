#include "optimizer.h"

namespace Engine::ModelsBuilder::Optimization {

AdamOptimizer::AdamOptimizer(float learning_rate)
    : learning_rate_(learning_rate), beta1_(0.9f), beta2_(0.999f) {
}

} // namespace Engine::ModelsBuilder::Optimization
