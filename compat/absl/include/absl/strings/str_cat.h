#pragma once

#include <sstream>
#include <string>
#include <type_traits>

#include "absl/strings/string_view.h"

namespace absl {
namespace compat {

inline void AppendOne(std::string* out, string_view value) {
    out->append(value.data(), value.size());
}

inline void AppendOne(std::string* out, const char* value) {
    if (value != nullptr) {
        out->append(value);
    }
}

inline void AppendOne(std::string* out, char* value) {
    AppendOne(out, static_cast<const char*>(value));
}

inline void AppendOne(std::string* out, char value) {
    out->push_back(value);
}

template <typename T>
inline void AppendOne(std::string* out, const T& value) {
    std::ostringstream stream;
    stream << value;
    out->append(stream.str());
}

}  // namespace compat

template <typename... Args>
inline std::string StrCat(const Args&... args) {
    std::string out;
    (compat::AppendOne(&out, args), ...);
    return out;
}

template <typename... Args>
inline void StrAppend(std::string* out, const Args&... args) {
    if (out == nullptr) {
        return;
    }
    (compat::AppendOne(out, args), ...);
}

}  // namespace absl