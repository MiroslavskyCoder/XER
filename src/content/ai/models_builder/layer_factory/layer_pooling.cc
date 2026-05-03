#include "layer_pooling.h"

namespace Engine::ModelsBuilder::LayerFactory {

LayerPooling::LayerPooling(const std::string& name, Config config)
    : Core::Layer(name), config_(config) {}

std::shared_ptr<Core::Layer> MakePooling(const std::string& name,
                                          LayerPooling::Config config) {
  return std::make_shared<LayerPooling>(name, config);
}

}  // namespace Engine::ModelsBuilder::LayerFactory
