#pragma once

#include <string>
#include <vector>

class EcoSystemManifestArgs {
public:
    // Split a shell-like argument line into argv tokens.
    // Supports basic single and double quotes.
    static std::vector<std::string> Split(const std::string& text);
};
