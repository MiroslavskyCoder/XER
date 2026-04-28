#include "xer/xer_string.h"

#include <absl/strings/ascii.h>
#include <absl/strings/str_cat.h>
#include <absl/strings/str_replace.h>
#include <range/v3/view/transform.hpp>

#if ENGINE_HAS_ICU
#include <unicode/normalizer2.h>
#include <unicode/unistr.h>
#endif

#include <vector>

namespace Xer {

std::string XerString::NormalizeSource(std::string_view source, std::string* error_out) {
    std::string normalized(source);
    if (normalized.size() >= 3
        && static_cast<unsigned char>(normalized[0]) == 0xEF
        && static_cast<unsigned char>(normalized[1]) == 0xBB
        && static_cast<unsigned char>(normalized[2]) == 0xBF) {
        normalized.erase(0, 3);
    }

#if ENGINE_HAS_ICU
    UErrorCode status = U_ZERO_ERROR;
    const icu::Normalizer2* normalizer = icu::Normalizer2::getNFCInstance(status);
    if (U_FAILURE(status) || normalizer == nullptr) {
        if (error_out != nullptr) {
            *error_out = "ICU normalizer initialization failed";
        }
        return normalized;
    }

    icu::UnicodeString source_unicode = icu::UnicodeString::fromUTF8(normalized);
    icu::UnicodeString output_unicode;
    normalizer->normalize(source_unicode, output_unicode, status);
    if (U_FAILURE(status)) {
        if (error_out != nullptr) {
            *error_out = "ICU source normalization failed";
        }
        return normalized;
    }

    std::string utf8;
    output_unicode.toUTF8String(utf8);
    return utf8;
#else
    if (error_out != nullptr) {
        error_out->clear();
    }
    return normalized;
#endif
}

std::string XerString::ToLowerAscii(std::string_view text) {
    std::string lowered(text);
    absl::AsciiStrToLower(&lowered);
    return lowered;
}

std::string XerString::ExpandTemplate(
    std::string_view format,
    const absl::flat_hash_map<std::string, std::string>& replacements) {
    std::vector<std::pair<std::string, std::string>> replace_pairs;
    const auto replacement_view = replacements | ranges::views::transform([](const auto& entry) {
        return std::pair<std::string, std::string>(
            absl::StrCat("%", entry.first, "%"),
            entry.second);
    });
    for (const auto& replacement : replacement_view) {
        replace_pairs.push_back(replacement);
    }

    return absl::StrReplaceAll(std::string(format), replace_pairs);
}

}  // namespace Xer