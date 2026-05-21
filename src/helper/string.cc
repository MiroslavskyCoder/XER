#include "helper/string.h"

#include <algorithm>
#include <string>

#include <absl/strings/ascii.h>
#include <absl/strings/str_cat.h>
#include <absl/strings/strip.h>

#ifndef ENGINE_HAS_ICU
#define ENGINE_HAS_ICU 0
#endif

#if ENGINE_HAS_ICU
#include <unicode/normalizer2.h>
#include <unicode/unistr.h>
#include <unicode/utypes.h>
#endif

namespace Helper::String {

std::string NormalizeUtf8(absl::string_view text) {
#if ENGINE_HAS_ICU
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
#else
	return std::string(text);
#endif
}

bool IsBlank(absl::string_view text) {
	return std::all_of(text.begin(), text.end(), [](char ch) {
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
