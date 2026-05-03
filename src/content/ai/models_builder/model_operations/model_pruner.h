#pragma once

#include "../model_core/model.h"

#include <memory>

namespace Engine::ModelsBuilder::Operations {

class ModelPruner {
 public:
  std::shared_ptr<Core::Model> PruneLastLayers(const Core::Model& model,
											   size_t layers_to_prune) const;
  std::shared_ptr<Core::Model> KeepFirstLayers(const Core::Model& model,
											   size_t layers_to_keep) const;
};

}  // namespace Engine::ModelsBuilder::Operations

