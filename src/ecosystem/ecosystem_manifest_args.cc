#include "ecosystem/ecosystem_manifest_args.h"

#include <absl/strings/ascii.h>

std::vector<std::string> EcoSystemManifestArgs::Split(const std::string& text) {
    std::vector<std::string> out;
    std::string token;
    char quote = '\0';

    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];

        if (quote != '\0') {
            if (c == quote) {
                quote = '\0';
            } else {
                token.push_back(c);
            }
            continue;
        }

        if (c == '\'' || c == '"') {
            quote = c;
            continue;
        }

        if (absl::ascii_isspace(static_cast<unsigned char>(c))) {
            if (!token.empty()) {
                token = std::string(absl::StripAsciiWhitespace(token));
                out.push_back(token);
                token.clear();
            }
            continue;
        }

        token.push_back(c);
    }

    if (!token.empty()) {
        token = std::string(absl::StripAsciiWhitespace(token));
        out.push_back(token);
    }

    return out;
}
