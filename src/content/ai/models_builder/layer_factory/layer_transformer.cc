#include "layer_transformer.h"

namespace Engine::ModelsBuilder::LayerFactory {

LayerTransformer::LayerTransformer(const std::string& name, Config config)
    : Core::Layer(Core::LayerType::Dense), config_(config) {
  SetLayerName(name);
}

std::shared_ptr<Core::Layer> MakeTransformer(
    const std::string& name, LayerTransformer::Config config) {
  return std::make_shared<LayerTransformer>(name, config);
}

}  // namespace Engine::ModelsBuilder::LayerFactory
