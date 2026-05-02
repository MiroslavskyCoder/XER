#pragma once
#include <string>

namespace EngineDoctor {

class RemotePackageSource {
public:
    bool Download(const std::string& url, const std::string& dest_path);
};

}  // namespace EngineDoctor
