#pragma once

#include <string>

namespace qt6::core {

bool IsAvailable();
std::string VersionString();
std::string CleanPath(const std::string& value);
std::string ToNativeSeparators(const std::string& value);
std::string AbsolutePath(const std::string& value);
std::string DirName(const std::string& value);
std::string BaseName(const std::string& value);
std::string Extension(const std::string& value);
std::string JoinPath(const std::string& base, const std::string& child);
bool PathExists(const std::string& value);
bool IsDir(const std::string& value);

}  // namespace qt6::core