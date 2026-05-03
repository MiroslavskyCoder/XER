#include "layer_batch_norm.h"

namespace Engine::ModelsBuilder::LayerFactory {

LayerBatchNorm::LayerBatchNorm(const std::string& name, Config config)
    : Core::Layer(name), config_(config) {}

std::shared_ptr<Core::Layer> MakeBatchNorm(const std::string& name,
                                              LayerBatchNorm::Config config) {
  return std::make_shared<LayerBatchNorm>(name, config);
}

}  // namespace Engine::ModelsBuilder::LayerFactory
