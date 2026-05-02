#include "scanner/path_utils.h"

#include <filesystem>

namespace EngineDoctor {
namespace PathUtils {

std::string NormalizePath(std::string path) {
    return std::filesystem::path(std::move(path)).lexically_normal().string();
}

bool IsAbsolute(const std::string& path) {
    return std::filesystem::path(path).is_absolute();
}

std::string JoinPaths(const std::string& a, const std::string& b) {
    return (std::filesystem::path(a) / b).string();
}

std::string GetExtension(const std::string& path) {
    return std::filesystem::path(path).extension().string();
}

std::string GetBaseName(const std::string& path) {
    return std::filesystem::path(path).filename().string();
}

std::string GetDirectory(const std::string& path) {
    return std::filesystem::path(path).parent_path().string();
}

} // namespace PathUtils
} // namespace EngineDoctor
