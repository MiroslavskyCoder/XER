#pragma once

#include "ecosystem/ecosystem_manifest_model.h"

#include <string>

class EcoSystemManifestParser {
public:
    static bool Parse(const std::string& json_text,
                      EcoSystemManifest* out,
                      std::string* error_message);
};
