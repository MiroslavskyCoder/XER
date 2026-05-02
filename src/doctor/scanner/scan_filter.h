#pragma once

#include <string>
#include <vector>

namespace EngineDoctor {

class ScanFilter {
public:
    void AddIncludePattern(std::string pattern);
    void AddExcludePattern(std::string pattern);
    void AddExtension(std::string ext);
    bool Matches(const std::string& path) const;

private:
    std::vector<std::string> include_patterns_;
    std::vector<std::string> exclude_patterns_;
    std::vector<std::string> extensions_;
};

} // namespace EngineDoctor
