#pragma once
#include <string>
#include <unordered_set>

namespace EngineDoctor {

class ScanAllowPackagesArch {
public:
    void AddAllowed(const std::string& name);
    bool IsAllowed(const std::string& name) const;

private:
    std::unordered_set<std::string> allowed_;
};

}  // namespace EngineDoctor
