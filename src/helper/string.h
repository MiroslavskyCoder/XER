#pragma once

#include <string>

#include <absl/strings/string_view.h>

namespace Helper::String {

std::string NormalizeUtf8(absl::string_view text);

bool IsBlank(absl::string_view text);

std::string TrimAsciiWhitespaceCopy(absl::string_view text);

std::string CanonicalizeToken(absl::string_view text);

std::string DefaultString(absl::string_view value, absl::string_view fallback);

std::string BuildKeyValueLine(absl::string_view key, absl::string_view value);

}  // namespace Helper::String
