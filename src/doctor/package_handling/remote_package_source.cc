#include "package_handling/remote_package_source.h"
#include <cstdlib>
#include <string>

namespace EngineDoctor {

bool RemotePackageSource::Download(const std::string& url,
                                   const std::string& dest_path) {
    const std::string cmd = "curl -L -o " + dest_path + " " + url;
    return std::system(cmd.c_str()) == 0;
}

}  // namespace EngineDoctor
