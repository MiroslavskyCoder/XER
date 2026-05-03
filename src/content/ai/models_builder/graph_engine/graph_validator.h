#pragma once

#include "graph_base.h"
#include <string>
#include <vector>

namespace Engine::ModelsBuilder::GraphEngine {

struct ValidationError {
  std::string message;
};

/// @brief Validates graph for cycles, unresolved inputs, and duplicate names
class GraphValidator {
 public:
  /// @return Empty vector if graph is valid; otherwise list of errors
  static std::vector<ValidationError> Validate(const GraphBase& graph);

 private:
  static bool HasCycle(const GraphBase& graph);
};

}  // namespace Engine::ModelsBuilder::GraphEngine
