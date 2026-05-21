#pragma once

#include <charconv>
#include <string>
#include <type_traits>

#include "absl/strings/strip.h"
#include "absl/strings/string_view.h"

namespace absl {

template <typename Integer>
inline bool SimpleAtoi(string_view text, Integer* out) {
    static_assert(std::is_integral_v<Integer>, "SimpleAtoi requires an integral output type");
    if (out == nullptr) {
        return false;
    }
    text = StripAsciiWhitespace(text);
    Integer value{};
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    const auto result = std::from_chars(begin, end, value);
    if (result.ec != std::errc() || result.ptr != end) {
        return false;
    }
    *out = value;
    return true;
}

}  // namespace absl