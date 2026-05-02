#include "package_handling/package_installer.h"
#include <cstdlib>
#include <string>

namespace EngineDoctor {

bool PackageInstaller::Install(const std::string& name) {
    const std::string cmd = "apt-get install -y " + name;
    return std::system(cmd.c_str()) == 0;
}

bool PackageInstaller::Uninstall(const std::string& name) {
    const std::string cmd = "apt-get remove -y " + name;
    return std::system(cmd.c_str()) == 0;
}

}  // namespace EngineDoctor
