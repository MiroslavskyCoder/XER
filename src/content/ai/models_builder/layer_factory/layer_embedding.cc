#include "layer_embedding.h"

namespace Engine::ModelsBuilder::LayerFactory {

LayerEmbedding::LayerEmbedding(const std::string& name, Config config)
    : Core::Layer(name), config_(config) {}

std::shared_ptr<Core::Layer> MakeEmbedding(const std::string& name,
                                              LayerEmbedding::Config config) {
  return std::make_shared<LayerEmbedding>(name, config);
}

}  // namespace Engine::ModelsBuilder::LayerFactory
