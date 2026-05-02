#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace EngineDoctor {

class DependencyGraph {
public:
    void AddEdge(const std::string& from, const std::string& to);
    bool HasEdge(const std::string& from, const std::string& to) const;
    std::vector<std::string> GetDependencies(const std::string& node) const;
    std::vector<std::string> GetAllNodes() const;
    std::vector<std::string> TopoSort() const;

private:
    std::unordered_map<std::string, std::vector<std::string>> adj_;
};

} // namespace EngineDoctor
