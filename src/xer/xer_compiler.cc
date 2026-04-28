#include "xer/xer_compiler.h"

#include "xer/xer_contrustor.h"
#include "xer/xer_util.h"

#include <absl/strings/str_format.h>

namespace Xer {

namespace {

bool IsFunctionStart(const std::vector<XerToken>& tokens, std::size_t index) {
    std::size_t offset = 0;
    if (index + offset < tokens.size() && tokens[index + offset].kind == XerTokenKind::kAsync) {
        ++offset;
    }
    return index + offset + 4 < tokens.size()
        && tokens[index + offset].kind == XerTokenKind::kIdentifier
        && tokens[index + offset + 1].kind == XerTokenKind::kLParen
        && tokens[index + offset + 2].kind == XerTokenKind::kRParen
        && tokens[index + offset + 3].kind == XerTokenKind::kArrow
        && tokens[index + offset + 4].kind == XerTokenKind::kThen;
}

}  // namespace

XerCompileResult XerCompiler::CompileFromText(std::string_view source, std::string* error_out) const {
    XerCompileResult result;
    result.normalized_source = XerString::NormalizeSource(source, error_out);

    XerTokenzer tokenzer;
    result.tokens = tokenzer.Tokenize(result.normalized_source, error_out);
    if (result.tokens.empty()) {
        return result;
    }

    XerContrustor contrustor;
    result.program.metadata = XerValue::MakeObject();

    for (std::size_t index = 0; index < result.tokens.size(); ++index) {
        const XerToken& token = result.tokens[index];

        if (token.kind == XerTokenKind::kImport
            && index + 5 < result.tokens.size()
            && result.tokens[index + 1].kind == XerTokenKind::kLParen
            && result.tokens[index + 2].kind == XerTokenKind::kString
            && result.tokens[index + 3].kind == XerTokenKind::kRParen
            && result.tokens[index + 4].kind == XerTokenKind::kAs
            && result.tokens[index + 5].kind == XerTokenKind::kIdentifier) {
            result.program.imports.push_back(contrustor.MakeImport(
                result.tokens[index + 2].text,
                result.tokens[index + 5].text,
                XerSourceRange{token.range.begin, result.tokens[index + 5].range.end}));
            continue;
        }

        if (IsFunctionStart(result.tokens, index)) {
            std::size_t offset = 0;
            const bool is_async = result.tokens[index].kind == XerTokenKind::kAsync;
            if (is_async) {
                ++offset;
            }
            XerAstFunction function = contrustor.MakeFunction(
                result.tokens[index + offset].text,
                is_async,
                XerSourceRange{result.tokens[index].range.begin, result.tokens[index + offset + 4].range.end});

            std::size_t body = index + offset + 5;
            while (body < result.tokens.size() && result.tokens[body].kind != XerTokenKind::kEnd) {
                if (result.tokens[body].kind == XerTokenKind::kLocal
                    && body + 1 < result.tokens.size()
                    && result.tokens[body + 1].kind == XerTokenKind::kIdentifier) {
                    contrustor.AddLocal(&function, result.tokens[body + 1].text);
                }

                if (result.tokens[body].kind == XerTokenKind::kIdentifier) {
                    if (body + 1 < result.tokens.size() && result.tokens[body + 1].kind == XerTokenKind::kLParen) {
                        contrustor.AddCall(&function, contrustor.MakeCall(
                            result.tokens[body].text,
                            std::nullopt,
                            result.tokens[body].range));
                    } else if (body + 2 < result.tokens.size()
                               && result.tokens[body + 1].kind == XerTokenKind::kColon
                               && result.tokens[body + 2].kind == XerTokenKind::kIdentifier) {
                        contrustor.AddCall(&function, contrustor.MakeCall(
                            result.tokens[body + 2].text,
                            result.tokens[body].text,
                            XerSourceRange{result.tokens[body].range.begin, result.tokens[body + 2].range.end}));
                    }
                }
                ++body;
            }

            result.program.functions.push_back(std::move(function));
            index = body;
            continue;
        }

        if (token.kind == XerTokenKind::kIdentifier && index + 1 < result.tokens.size() && result.tokens[index + 1].kind == XerTokenKind::kLParen) {
            contrustor.AddTopLevelCall(&result.program, contrustor.MakeCall(token.text, std::nullopt, token.range));
        }
    }

    if (auto metadata = result.program.metadata.AsObject()) {
        (*metadata)["token_count"] = XerValue(static_cast<std::int64_t>(result.tokens.size()));
        (*metadata)["function_count"] = XerValue(static_cast<std::int64_t>(result.program.functions.size()));
        (*metadata)["import_count"] = XerValue(static_cast<std::int64_t>(result.program.imports.size()));
    }

    if (error_out != nullptr && error_out->empty()) {
        *error_out = absl::StrFormat(
            "compiled %zu tokens, %zu imports, %zu functions",
            result.tokens.size(),
            result.program.imports.size(),
            result.program.functions.size());
    }

    return result;
}

XerCompileResult XerCompiler::CompileFromFile(const std::filesystem::path& path, std::string* error_out) const {
    const std::string source = XerUtil::ReadSourceFile(path, error_out);
    if (source.empty() && !std::filesystem::exists(path)) {
        return XerCompileResult();
    }
    return CompileFromText(source, error_out);
}

}  // namespace Xer