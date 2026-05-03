#include "layer_attention.h"

namespace Engine::ModelsBuilder::LayerFactory {

LayerAttention::LayerAttention(const std::string& name, Config config)
    : Core::Layer(name), config_(config) {}

std::shared_ptr<Core::Layer> MakeAttention(const std::string& name,
                                              LayerAttention::Config config) {
  return std::make_shared<LayerAttention>(name, config);
}

}  // namespace Engine::ModelsBuilder::LayerFactory
