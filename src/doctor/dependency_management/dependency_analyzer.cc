#include "dependency_management/dependency_analyzer.h"
#include <fstream>
#include <regex>
#include <sstream>

namespace EngineDoctor {

void DependencyAnalyzer::ParseIncludes(const std::string& content,
                                       std::vector<std::string>& includes) {
    // Match both #include "..." and #include <...>
    static const std::regex include_re(R"(#\s*include\s*[<"]([^>"]+)[>"])");
    auto begin = std::sregex_iterator(content.begin(), content.end(), include_re);
    auto end   = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        includes.push_back((*it)[1].str());
    }
}

DependencyInfo DependencyAnalyzer::Analyze(const std::string& file_path) {
    DependencyInfo info;
    info.file = file_path;

    std::ifstream f(file_path);
    if (!f.is_open()) return info;

    std::ostringstream ss;
    ss << f.rdbuf();
    std::string content = ss.str();

    ParseIncludes(content, info.includes);
    return info;
}

} // namespace EngineDoctor
