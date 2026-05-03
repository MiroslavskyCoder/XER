#include "layer_conv3d.h"

namespace Engine::ModelsBuilder::LayerFactory {

LayerConv3D::LayerConv3D(const std::string& name, Config config)
    : Core::Layer(name), config_(config) {}

std::shared_ptr<Core::Layer> MakeConv3D(const std::string& name,
                                         LayerConv3D::Config config) {
  return std::make_shared<LayerConv3D>(name, config);
}

}  // namespace Engine::ModelsBuilder::LayerFactory
