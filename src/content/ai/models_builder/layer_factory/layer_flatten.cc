#include "layer_flatten.h"

namespace Engine::ModelsBuilder::LayerFactory {

LayerFlatten::LayerFlatten(const std::string& name, int start_dim, int end_dim)
    : Core::Layer(name), start_dim_(start_dim), end_dim_(end_dim) {}

std::shared_ptr<Core::Layer> MakeFlatten(const std::string& name,
                                           int start_dim, int end_dim) {
  return std::make_shared<LayerFlatten>(name, start_dim, end_dim);
}

}  // namespace Engine::ModelsBuilder::LayerFactory
