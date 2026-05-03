#include "layer_linear.h"

namespace Engine::ModelsBuilder::LayerFactory {

LayerLinear::LayerLinear(const std::string& name, Config config)
    : Core::Layer(name), config_(config) {}

std::shared_ptr<Core::Layer> MakeLinear(const std::string& name,
                                          LayerLinear::Config config) {
  return std::make_shared<LayerLinear>(name, config);
}

}  // namespace Engine::ModelsBuilder::LayerFactory
