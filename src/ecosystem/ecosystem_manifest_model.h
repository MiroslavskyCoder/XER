#pragma once

#include <string>
#include <vector>

struct EcoSystemManifest {
    std::string script_path;
    std::vector<std::string> args;

    bool IsValid() const { return !script_path.empty(); }
};
