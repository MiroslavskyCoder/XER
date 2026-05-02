#pragma once

#include <string>

namespace EngineDoctor {
namespace PathUtils {

std::string NormalizePath(std::string path);
bool IsAbsolute(const std::string& path);
std::string JoinPaths(const std::string& a, const std::string& b);
std::string GetExtension(const std::string& path);
std::string GetBaseName(const std::string& path);
std::string GetDirectory(const std::string& path);

} // namespace PathUtils
} // namespace EngineDoctor
