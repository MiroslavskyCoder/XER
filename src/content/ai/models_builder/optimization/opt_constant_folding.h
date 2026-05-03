#pragma once

#include "../graph_engine/graph_base.h"

namespace Engine::ModelsBuilder::Optimization {

/// @brief Folds constant sub-expressions at graph-compile time
class OptConstantFolding {
 public:
  static void Apply(GraphEngine::GraphBase& graph);
};

}  // namespace Engine::ModelsBuilder::Optimization
