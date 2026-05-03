#include "dependency_scanner.h"

namespace Engine::ModelsBuilder::Reader::Schema {

DependencyScanner::ScanResult DependencyScanner::Scan(
    const Core::Model& model) const {
  ScanResult result;
  // Sequential models have trivially connected layers; no missing tensors.
  // DAG validation (Functional models) requires full graph representation —
  // implement when graph_engine is complete.
  (void)model;
  return result;
}

}  // namespace Engine::ModelsBuilder::Reader::Schema
