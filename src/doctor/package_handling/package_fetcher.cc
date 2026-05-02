#include "package_handling/package_fetcher.h"
#include <array>
#include <cstdio>
#include <string>

namespace EngineDoctor {

bool PackageFetcher::Fetch(const std::string& name, PackageInfo& out) {
    const std::string cmd = "apt-cache show " + name + " 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return false;

    std::array<char, 256> buf{};
    bool found = false;
    while (fgets(buf.data(), static_cast<int>(buf.size()), pipe)) {
        std::string line(buf.data());
        if (line.rfind("Package: ", 0) == 0) {
            out.name = line.substr(9);
            if (!out.name.empty() && out.name.back() == '\n')
                out.name.pop_back();
            found = true;
        } else if (line.rfind("Version: ", 0) == 0) {
            out.version = line.substr(9);
            if (!out.version.empty() && out.version.back() == '\n')
                out.version.pop_back();
        } else if (line.rfind("Architecture: ", 0) == 0) {
            out.arch = line.substr(14);
            if (!out.arch.empty() && out.arch.back() == '\n')
                out.arch.pop_back();
        } else if (line.rfind("Description: ", 0) == 0) {
            out.description = line.substr(13);
            if (!out.description.empty() && out.description.back() == '\n')
                out.description.pop_back();
        }
    }

    pclose(pipe);
    return found;
}

}  // namespace EngineDoctor
