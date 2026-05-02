#pragma once

#include <string>
#include <unordered_set>

namespace EngineDoctor {

class ScanContext {
public:
    std::string root_path;
    int current_depth = 0;

    void MarkVisited(const std::string& path);
    bool IsVisited(const std::string& path) const;
    void Reset();

private:
    std::unordered_set<std::string> visited_;
};

} // namespace EngineDoctor
