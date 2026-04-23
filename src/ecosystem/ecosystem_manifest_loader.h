#pragma once

#include "ecosystem/ecosystem_manifest_model.h"

#include <string>
#include <vector>

class EcoSystemManifestLoader {
public:
    explicit EcoSystemManifestLoader(std::string manifest_path);

    bool Load(EcoSystemManifest* out, std::string* error_message) const;

    // Build argv-like vector:
    // ["engine", "run", <script>, <args...>]
    static std::vector<std::string> BuildArgv(const EcoSystemManifest& manifest);

private:
    std::string manifest_path_;
};
