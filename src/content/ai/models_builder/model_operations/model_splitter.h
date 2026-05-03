#pragma once

#include "../model_core/model.h"

#include <memory>
#include <utility>

namespace Engine::ModelsBuilder::Operations {

class ModelSplitter {
 public:
  std::pair<std::shared_ptr<Core::Model>, std::shared_ptr<Core::Model>> Split(
	  const Core::Model& model,
	  size_t split_layer_index) const;
};

}  // namespace Engine::ModelsBuilder::Operations

