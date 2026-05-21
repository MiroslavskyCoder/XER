#pragma once

#include <string>
#include <vector>

#include "absl/strings/strip.h"
#include "absl/strings/string_view.h"

namespace absl {

struct ByCharDelimiter {
    char delimiter;
};

struct SkipEmptyTag {};
struct SkipWhitespaceTag {};

inline ByCharDelimiter ByChar(char delimiter) {
    return ByCharDelimiter{delimiter};
}

inline SkipEmptyTag SkipEmpty() {
    return SkipEmptyTag{};
}

inline SkipWhitespaceTag SkipWhitespace() {
    return SkipWhitespaceTag{};
}

namespace compat {

inline std::vector<std::string> SplitImpl(string_view input, char delimiter, bool skip_empty, bool trim) {
    std::vector<std::string> out;
    size_t start = 0;
    while (start <= input.size()) {
        const size_t end = input.find(delimiter, start);
        string_view token = end == string_view::npos
            ? input.substr(start)
            : input.substr(start, end - start);
        if (trim) {
            token = StripAsciiWhitespace(token);
        }
        if (!skip_empty || !token.empty()) {
            out.emplace_back(token);
        }
        if (end == string_view::npos) {
            break;
        }
        start = end + 1;
    }
    return out;
}

}  // namespace compat

inline std::vector<std::string> StrSplit(string_view input, char delimiter) {
    return compat::SplitImpl(input, delimiter, false, false);
}

inline std::vector<std::string> StrSplit(string_view input, char delimiter, SkipWhitespaceTag) {
    return compat::SplitImpl(input, delimiter, true, true);
}

inline std::vector<std::string> StrSplit(string_view input, ByCharDelimiter delimiter, SkipEmptyTag) {
    return compat::SplitImpl(input, delimiter.delimiter, true, false);
}

}  // namespace absl