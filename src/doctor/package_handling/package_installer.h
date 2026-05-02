#pragma once
#include <string>

namespace EngineDoctor {

class PackageInstaller {
public:
    bool Install(const std::string& name);
    bool Uninstall(const std::string& name);
};

}  // namespace EngineDoctor
