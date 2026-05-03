#include "opt_dead_node_remover.h"

#include "../graph_engine/graph_optimizer.h"

namespace Engine::ModelsBuilder::Optimization {

void OptDeadNodeRemover::Apply(GraphEngine::GraphBase& graph) {
  GraphEngine::GraphOptimizer::PruneDeadNodes(graph);
}

}  // namespace Engine::ModelsBuilder::Optimization
