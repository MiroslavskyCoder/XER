#pragma once
#include "dependency_management/dependency_info.h"
#include <string>
#include <vector>

namespace EngineDoctor {

class DependencyAnalyzer {
public:
    DependencyInfo Analyze(const std::string& file_path);

private:
    void ParseIncludes(const std::string& content, std::vector<std::string>& includes);
};

} // namespace EngineDoctor
