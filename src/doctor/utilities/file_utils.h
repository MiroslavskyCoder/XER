#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace EngineDoctor {
namespace FileUtils {

std::string ReadFile(const std::string& path);
bool WriteFile(const std::string& path, const std::string& content);
bool FileExists(const std::string& path);
std::vector<std::string> ListDir(const std::string& dir);
std::uintmax_t GetFileSize(const std::string& path);

}  // namespace FileUtils
}  // namespace EngineDoctor
