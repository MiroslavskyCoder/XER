#pragma once

#include <cstdio>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "absl/strings/string_view.h"

namespace absl {
namespace compat {

template <typename T, typename Enable = void>
class FormatArgStorage {
public:
    explicit FormatArgStorage(const T& value) {
        std::ostringstream stream;
        stream << value;
        text_ = stream.str();
    }

    const char* value() const { return text_.c_str(); }

private:
    std::string text_;
};

template <typename T>
class FormatArgStorage<T, std::enable_if_t<std::is_arithmetic_v<std::decay_t<T>>>> {
public:
    explicit FormatArgStorage(T value) : value_(value) {}
    std::decay_t<T> value() const { return value_; }

private:
    std::decay_t<T> value_;
};

template <typename T>
class FormatArgStorage<T, std::enable_if_t<
    std::is_same_v<std::decay_t<T>, const char*> || std::is_same_v<std::decay_t<T>, char*>>> {
public:
    explicit FormatArgStorage(T value) : value_(value != nullptr ? value : "") {}
    const char* value() const { return value_; }

private:
    const char* value_;
};

template <>
class FormatArgStorage<std::string, void> {
public:
    explicit FormatArgStorage(const std::string& value) : value_(value) {}
    const char* value() const { return value_.c_str(); }

private:
    std::string value_;
};

template <>
class FormatArgStorage<string_view, void> {
public:
    explicit FormatArgStorage(string_view value) : value_(value) {}
    const char* value() const { return value_.c_str(); }

private:
    std::string value_;
};

template <typename Tuple, size_t... Index>
inline std::string FormatWithTuple(const std::string& format, const Tuple& storage, std::index_sequence<Index...>) {
    const int size = std::snprintf(nullptr, 0, format.c_str(), std::get<Index>(storage).value()...);
    if (size <= 0) {
        return format;
    }
    std::vector<char> buffer(static_cast<size_t>(size) + 1u);
    std::snprintf(buffer.data(), buffer.size(), format.c_str(), std::get<Index>(storage).value()...);
    return std::string(buffer.data(), static_cast<size_t>(size));
}

}  // namespace compat

template <typename... Args>
inline std::string StrFormat(const std::string& format, const Args&... args) {
    auto storage = std::make_tuple(compat::FormatArgStorage<std::decay_t<Args>>(args)...);
    return compat::FormatWithTuple(format, storage, std::index_sequence_for<Args...>{});
}

template <typename... Args>
inline void StrAppendFormat(std::string* out, const std::string& format, const Args&... args) {
    if (out != nullptr) {
        out->append(StrFormat(format, args...));
    }
}

}  // namespace absl