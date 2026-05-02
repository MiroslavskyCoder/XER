#include "dependency_management/dependency_graph.h"
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <unordered_set>

namespace EngineDoctor {

void DependencyGraph::AddEdge(const std::string& from, const std::string& to) {
    adj_[from].push_back(to);
    // Ensure destination node exists even with no outgoing edges
    if (adj_.find(to) == adj_.end()) {
        adj_[to] = {};
    }
}

bool DependencyGraph::HasEdge(const std::string& from, const std::string& to) const {
    auto it = adj_.find(from);
    if (it == adj_.end()) return false;
    return std::find(it->second.begin(), it->second.end(), to) != it->second.end();
}

std::vector<std::string> DependencyGraph::GetDependencies(const std::string& node) const {
    auto it = adj_.find(node);
    if (it == adj_.end()) return {};
    return it->second;
}

std::vector<std::string> DependencyGraph::GetAllNodes() const {
    std::vector<std::string> nodes;
    nodes.reserve(adj_.size());
    for (const auto& [node, _] : adj_) {
        nodes.push_back(node);
    }
    return nodes;
}

std::vector<std::string> DependencyGraph::TopoSort() const {
    // 0 = unvisited, 1 = visiting, 2 = visited
    std::unordered_map<std::string, int> state;
    std::vector<std::string> result;

    std::function<void(const std::string&)> dfs = [&](const std::string& node) {
        int& s = state[node];
        if (s == 2) return;
        if (s == 1) throw std::runtime_error("Cycle detected in dependency graph at: " + node);
        s = 1;
        auto it = adj_.find(node);
        if (it != adj_.end()) {
            for (const auto& dep : it->second) {
                dfs(dep);
            }
        }
        s = 2;
        result.push_back(node);
    };

    for (const auto& [node, _] : adj_) {
        if (state[node] == 0) {
            dfs(node);
        }
    }

    std::reverse(result.begin(), result.end());
    return result;
}

} // namespace EngineDoctor
