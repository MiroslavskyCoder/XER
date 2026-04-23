#pragma once

#include <string>

namespace engine::javascript::common {

class TextNormalizer {
public:
    static std::string NormalizeTopic(const std::string& value);
    static std::string NormalizeText(const std::string& value);
};

}  // namespace engine::javascript::common
