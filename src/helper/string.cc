#include "helper/string.h"

#include <string>

#include <absl/strings/ascii.h>
#include <absl/strings/str_cat.h>
#include <absl/strings/strip.h>
#include <range/v3/algorithm/all_of.hpp>
#include <unicode/normalizer2.h>
#include <unicode/unistr.h>
#include <unicode/utypes.h>

namespace Helper::String {

std::string NormalizeUtf8(absl::string_view text) {
	UErrorCode status = U_ZERO_ERROR;
	const icu::Normalizer2* normalizer = icu::Normalizer2::getNFCInstance(status);
	if (U_FAILURE(status) || normalizer == nullptr) {
		return std::string(text);
	}

	icu::UnicodeString source = icu::UnicodeString::fromUTF8(text);
	icu::UnicodeString normalized;
	normalizer->normalize(source, normalized, status);
	if (U_FAILURE(status)) {
		return std::string(text);
	}

	std::string out;
	normalized.toUTF8String(out);
	return out;
}

bool IsBlank(absl::string_view text) {
	return ranges::all_of(text, [](char ch) {
		return absl::ascii_isspace(static_cast<unsigned char>(ch));
	});
}

std::string TrimAsciiWhitespaceCopy(absl::string_view text) {
	return std::string(absl::StripAsciiWhitespace(text));
}

std::string CanonicalizeToken(absl::string_view text) {
	std::string normalized = NormalizeUtf8(TrimAsciiWhitespaceCopy(text));
	absl::AsciiStrToLower(&normalized);
	return normalized;
}

std::string DefaultString(absl::string_view value, absl::string_view fallback) {
	return IsBlank(value) ? std::string(fallback) : NormalizeUtf8(value);
}

std::string BuildKeyValueLine(absl::string_view key, absl::string_view value) {
	return absl::StrCat(NormalizeUtf8(key), "=", NormalizeUtf8(value));
}

}  // namespace Helper::String
