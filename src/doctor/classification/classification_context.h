#pragma once

#include <string>
#include <unordered_map>

namespace EngineDoctor {

class ClassificationContext {
public:
    std::string file_path;
    std::unordered_map<std::string, std::string> metadata;

    void Set(const std::string& key, const std::string& val);
    std::string Get(const std::string& key) const;
    bool HasKey(const std::string& key) const;
};

} // namespace EngineDoctor
