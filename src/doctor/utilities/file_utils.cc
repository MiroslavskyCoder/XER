#include "utilities/file_utils.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

namespace EngineDoctor {
namespace FileUtils {

std::string ReadFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool WriteFile(const std::string& path, const std::string& content) {
    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << content;
    return f.good();
}

bool FileExists(const std::string& path) {
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

std::vector<std::string> ListDir(const std::string& dir) {
    std::vector<std::string> entries;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (ec) break;
        entries.push_back(entry.path().filename().string());
    }
    return entries;
}

std::uintmax_t GetFileSize(const std::string& path) {
    std::error_code ec;
    return std::filesystem::file_size(path, ec);
}

}  // namespace FileUtils
}  // namespace EngineDoctor
