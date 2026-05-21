#pragma once

#include <algorithm>
#include <cctype>
#include <string>

#include "absl/strings/string_view.h"

namespace absl {

inline bool ascii_isspace(unsigned char ch) {
    return std::isspace(ch) != 0;
}

inline void AsciiStrToLower(std::string* value) {
    if (value == nullptr) {
        return;
    }
    std::transform(value->begin(), value->end(), value->begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
}

inline void AsciiStrToLower(std::string& value) {
    AsciiStrToLower(&value);
}

inline std::string AsciiStrToLower(string_view value) {
    std::string out(value);
    AsciiStrToLower(&out);
    return out;
}

}  // namespace absl