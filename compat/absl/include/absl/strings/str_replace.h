#pragma once

#include <initializer_list>
#include <string>
#include <utility>

#include "absl/strings/string_view.h"

namespace absl {

using ReplacementPair = std::pair<string_view, string_view>;

inline void ReplaceAllInPlace(std::string* input, string_view needle, string_view replacement) {
    if (input == nullptr || needle.empty()) {
        return;
    }
    size_t pos = 0;
    while ((pos = input->find(needle.data(), pos, needle.size())) != std::string::npos) {
        input->replace(pos, needle.size(), replacement.data(), replacement.size());
        pos += replacement.size();
    }
}

inline std::string StrReplaceAll(std::string input, std::initializer_list<ReplacementPair> replacements) {
    for (const auto& replacement : replacements) {
        ReplaceAllInPlace(&input, replacement.first, replacement.second);
    }
    return input;
}

template <typename Range>
inline std::string StrReplaceAll(std::string input, const Range& replacements) {
    for (const auto& replacement : replacements) {
        ReplaceAllInPlace(&input, string_view(replacement.first), string_view(replacement.second));
    }
    return input;
}

}  // namespace absl