#pragma once

#include "absl/strings/ascii.h"
#include "absl/strings/string_view.h"

namespace absl {

inline string_view StripAsciiWhitespace(string_view value) {
    while (!value.empty() && ascii_isspace(static_cast<unsigned char>(value.front()))) {
        value.remove_prefix(1);
    }
    while (!value.empty() && ascii_isspace(static_cast<unsigned char>(value.back()))) {
        value.remove_suffix(1);
    }
    return value;
}

}  // namespace absl