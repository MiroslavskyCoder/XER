#include "runtime_live.h"

#include <filesystem>
#include <system_error>

namespace FileSystem {

std::string CreateDirectory(const std::string& path) {
    std::filesystem::path directory(path);
    std::error_code ec;
    std::filesystem::create_directories(directory, ec);
    if (ec) {
        return std::string();
    }

    return std::filesystem::absolute(directory).string();
}

}  // namespace FileSystem
