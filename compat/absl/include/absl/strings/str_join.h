#pragma once

#include <string>

#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"

namespace absl {

template <typename Range>
inline std::string StrJoin(const Range& range, string_view delimiter) {
    std::string out;
    bool first = true;
    for (const auto& item : range) {
        if (!first) {
            out.append(delimiter.data(), delimiter.size());
        }
        first = false;
        StrAppend(&out, item);
    }
    return out;
}

}  // namespace absl