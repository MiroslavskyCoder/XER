#pragma once
#include <string>

namespace EngineDoctor {

class PackageDownloader {
public:
    bool Download(const std::string& url, const std::string& dest);
};

}  // namespace EngineDoctor
