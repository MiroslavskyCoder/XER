#include "xer/xer_tokenzer.h"

#include <absl/strings/str_format.h>

#include <cctype>

namespace Xer {

namespace {

class Lexer {
public:
    explicit Lexer(std::string source) : source_(std::move(source)) {}

    std::vector<XerToken> Tokenize() {
        std::vector<XerToken> tokens;
        while (!AtEnd()) {
            SkipWhitespaceAndComments();
            if (AtEnd()) {
                break;
            }

            if (std::isalpha(static_cast<unsigned char>(Peek())) || Peek() == '_') {
                tokens.push_back(LexIdentifier());
                continue;
            }
            if (std::isdigit(static_cast<unsigned char>(Peek()))) {
                tokens.push_back(LexNumber());
                continue;
            }

            switch (Peek()) {
                case '@': tokens.push_back(LexImport()); break;
                case '(': tokens.push_back(MakeSingleCharToken(XerTokenKind::kLParen)); break;
                case ')': tokens.push_back(MakeSingleCharToken(XerTokenKind::kRParen)); break;
                case '{': tokens.push_back(MakeSingleCharToken(XerTokenKind::kLBrace)); break;
                case '}': tokens.push_back(MakeSingleCharToken(XerTokenKind::kRBrace)); break;
                case ',': tokens.push_back(MakeSingleCharToken(XerTokenKind::kComma)); break;
                case ':': tokens.push_back(MakeSingleCharToken(XerTokenKind::kColon)); break;
                case ';': tokens.push_back(MakeSingleCharToken(XerTokenKind::kSemicolon)); break;
                case '=': tokens.push_back(MakeSingleCharToken(XerTokenKind::kEqual)); break;
                case '-': tokens.push_back(LexArrow()); break;
                case '"': tokens.push_back(LexString()); break;
                default:
                    throw std::runtime_error(absl::StrFormat(
                        "%zu:%zu: unexpected character '%c'",
                        location_.line,
                        location_.column,
                        Peek()));
            }
        }

        tokens.push_back(XerToken{XerTokenKind::kEndOfFile, std::string(), XerSourceRange{location_, location_}});
        return tokens;
    }

private:
    bool AtEnd() const { return index_ >= source_.size(); }

    char Peek() const { return AtEnd() ? '\0' : source_[index_]; }

    char PeekNext() const { return (index_ + 1) < source_.size() ? source_[index_ + 1] : '\0'; }

    char Advance() {
        const char ch = source_[index_++];
        if (ch == '\n') {
            ++location_.line;
            location_.column = 1;
        } else {
            ++location_.column;
        }
        return ch;
    }

    void SkipWhitespaceAndComments() {
        while (!AtEnd()) {
            if (std::isspace(static_cast<unsigned char>(Peek()))) {
                Advance();
                continue;
            }
            if (Peek() == '/' && PeekNext() == '/') {
                while (!AtEnd() && Peek() != '\n') {
                    Advance();
                }
                continue;
            }
            break;
        }
    }

    XerToken MakeSingleCharToken(XerTokenKind kind) {
        const XerSourceLocation begin = location_;
        const char ch = Advance();
        return XerToken{kind, std::string(1, ch), XerSourceRange{begin, location_}};
    }

    XerToken LexIdentifier() {
        const XerSourceLocation begin = location_;
        std::string text;
        while (!AtEnd()) {
            const char ch = Peek();
            if (!std::isalnum(static_cast<unsigned char>(ch)) && ch != '_') {
                break;
            }
            text.push_back(Advance());
        }

        XerTokenKind kind = XerTokenKind::kIdentifier;
        if (text == "as") kind = XerTokenKind::kAs;
        else if (text == "async") kind = XerTokenKind::kAsync;
        else if (text == "local") kind = XerTokenKind::kLocal;
        else if (text == "then") kind = XerTokenKind::kThen;
        else if (text == "end") kind = XerTokenKind::kEnd;

        return XerToken{kind, text, XerSourceRange{begin, location_}};
    }

    XerToken LexNumber() {
        const XerSourceLocation begin = location_;
        std::string text;
        while (!AtEnd() && std::isdigit(static_cast<unsigned char>(Peek()))) {
            text.push_back(Advance());
        }
        return XerToken{XerTokenKind::kNumber, text, XerSourceRange{begin, location_}};
    }

    XerToken LexImport() {
        const XerSourceLocation begin = location_;
        Advance();

        std::string keyword;
        while (!AtEnd() && std::isalpha(static_cast<unsigned char>(Peek()))) {
            keyword.push_back(Advance());
        }
        if (keyword != "import") {
            throw std::runtime_error(absl::StrFormat("%zu:%zu: unknown directive '@%s'", begin.line, begin.column, keyword));
        }
        return XerToken{XerTokenKind::kImport, "@import", XerSourceRange{begin, location_}};
    }

    XerToken LexArrow() {
        const XerSourceLocation begin = location_;
        Advance();
        if (Peek() != '>') {
            throw std::runtime_error(absl::StrFormat("%zu:%zu: expected '->'", begin.line, begin.column));
        }
        Advance();
        return XerToken{XerTokenKind::kArrow, "->", XerSourceRange{begin, location_}};
    }

    XerToken LexString() {
        const XerSourceLocation begin = location_;
        Advance();
        std::string text;
        while (!AtEnd()) {
            const char ch = Advance();
            if (ch == '"') {
                return XerToken{XerTokenKind::kString, text, XerSourceRange{begin, location_}};
            }
            if (ch == '\\' && !AtEnd()) {
                const char escaped = Advance();
                switch (escaped) {
                    case 'n': text.push_back('\n'); break;
                    case 'r': text.push_back('\r'); break;
                    case 't': text.push_back('\t'); break;
                    case '"': text.push_back('"'); break;
                    case '\\': text.push_back('\\'); break;
                    default: text.push_back(escaped); break;
                }
                continue;
            }
            text.push_back(ch);
        }

        throw std::runtime_error(absl::StrFormat("%zu:%zu: unterminated string literal", begin.line, begin.column));
    }

    std::string source_;
    std::size_t index_ = 0;
    XerSourceLocation location_;
};

}  // namespace

std::string XerTokenKindName(XerTokenKind kind) {
    switch (kind) {
        case XerTokenKind::kEndOfFile: return "eof";
        case XerTokenKind::kIdentifier: return "identifier";
        case XerTokenKind::kNumber: return "number";
        case XerTokenKind::kString: return "string";
        case XerTokenKind::kImport: return "import";
        case XerTokenKind::kAs: return "as";
        case XerTokenKind::kAsync: return "async";
        case XerTokenKind::kLocal: return "local";
        case XerTokenKind::kThen: return "then";
        case XerTokenKind::kEnd: return "end";
        case XerTokenKind::kLParen: return "(";
        case XerTokenKind::kRParen: return ")";
        case XerTokenKind::kLBrace: return "{";
        case XerTokenKind::kRBrace: return "}";
        case XerTokenKind::kComma: return ",";
        case XerTokenKind::kColon: return ":";
        case XerTokenKind::kSemicolon: return ";";
        case XerTokenKind::kEqual: return "=";
        case XerTokenKind::kArrow: return "->";
    }
    return "unknown";
}

std::vector<XerToken> XerTokenzer::Tokenize(std::string_view source, std::string* error_out) const {
    try {
        std::string normalization_error;
        const std::string normalized = XerString::NormalizeSource(source, &normalization_error);
        if (error_out != nullptr) {
            *error_out = normalization_error;
        }
        return Lexer(normalized).Tokenize();
    } catch (const std::exception& error) {
        if (error_out != nullptr) {
            *error_out = error.what();
        }
        return {};
    }
}

}  // namespace Xer