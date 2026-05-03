#include "trans_graph_cleaner.h"

namespace Engine::ModelsBuilder::Reader::Transform {

std::shared_ptr<Core::Model> TransGraphCleaner::Apply(
    std::shared_ptr<Core::Model> model) {
  auto clean = std::make_shared<Core::Model>(model->GetModelName());
  for (size_t i = 0; i < model->GetLayerCount(); ++i) {
    auto layer = model->GetLayer(i);
    if (layer) clean->AddLayer(layer);
  }
  return clean;
}

}  // namespace Engine::ModelsBuilder::Reader::Transform
