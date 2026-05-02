#pragma once
#include <string>
#include <vector>

namespace EngineDoctor {
namespace StringUtils {

std::string Trim(std::string s);
std::vector<std::string> Split(const std::string& s, char delim);
std::string ToLower(std::string s);
bool Contains(const std::string& s, const std::string& sub);
bool StartsWith(const std::string& s, const std::string& prefix);
bool EndsWith(const std::string& s, const std::string& suffix);
std::string Join(const std::vector<std::string>& parts, const std::string& sep);

}  // namespace StringUtils
}  // namespace EngineDoctor
