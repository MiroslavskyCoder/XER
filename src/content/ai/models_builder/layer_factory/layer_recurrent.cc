#include "layer_recurrent.h"

namespace Engine::ModelsBuilder::LayerFactory {

LayerRecurrent::LayerRecurrent(const std::string& name, Config config)
    : Core::Layer(name), config_(config) {}

std::shared_ptr<Core::Layer> MakeRecurrent(const std::string& name,
                                              LayerRecurrent::Config config) {
  return std::make_shared<LayerRecurrent>(name, config);
}

}  // namespace Engine::ModelsBuilder::LayerFactory
