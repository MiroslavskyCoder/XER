#include "fix_code_analyzer.h"

#include <regex>
#include <sstream>
#include <string>

namespace AutoCorrection {

namespace {

struct PatternRule {
    std::regex    pattern;
    std::string   message;
    IssueSeverity severity;
    FixKind       fix_kind;
    std::string   replacement; // For kAutoReplace: replacement text (empty = use match).
};

// Returns the shared list of lint rules.
const std::vector<PatternRule>& Rules() {
    static const std::vector<PatternRule> kRules = {
        // ── Dangerous patterns (errors) ──────────────────────────────────────
        { std::regex(R"(\beval\s*\()"),
          "eval() is dangerous and should not be used",
          IssueSeverity::kError, FixKind::kNone, "" },

        { std::regex(R"(\bnew\s+Function\s*\()"),
          "new Function() is equivalent to eval(); avoid it",
          IssueSeverity::kError, FixKind::kNone, "" },

        // ── Type-unsafe equality ─────────────────────────────────────────────
        { std::regex(R"([^=!<>]={2}[^=])"),
          "Use === instead of ==",
          IssueSeverity::kWarning, FixKind::kNone, "" },

        { std::regex(R"([^!]!=[^=])"),
          "Use !== instead of !=",
          IssueSeverity::kWarning, FixKind::kNone, "" },

        // ── Legacy declarations ──────────────────────────────────────────────
        { std::regex(R"(\bvar\s+)"),
          "Prefer let/const over var",
          IssueSeverity::kWarning, FixKind::kAutoReplace, "let " },

        // ── Debug artifacts ──────────────────────────────────────────────────
        { std::regex(R"(\bconsole\.log\s*\()"),
          "Remove debug console.log()",
          IssueSeverity::kWarning, FixKind::kNone, "" },

        { std::regex(R"(\bconsole\.warn\s*\()"),
          "Remove debug console.warn()",
          IssueSeverity::kWarning, FixKind::kNone, "" },

        { std::regex(R"(\bdebugger\s*;)"),
          "Remove debugger statement",
          IssueSeverity::kError, FixKind::kAutoRemove, "" },

        // ── Async / Promise issues ────────────────────────────────────────────
        { std::regex(R"(\.then\s*\(.*\.catch\s*\(.*=>\s*\{\}))"),
          "Empty .catch() silently swallows errors",
          IssueSeverity::kWarning, FixKind::kNone, "" },

        // ── Common mistake patterns ───────────────────────────────────────────
        { std::regex(R"(\btypeof\s+\w+\s*={2,3}\s*\"undefined\")"),
          "Prefer 'value === undefined' over typeof check",
          IssueSeverity::kWarning, FixKind::kNone, "" },

        { std::regex(R"(\bsetTimeout\s*\(\s*\w+\s*,\s*0\s*\))"),
          "setTimeout(fn, 0) is a code smell; consider queueMicrotask() or Promise.resolve()",
          IssueSeverity::kWarning, FixKind::kNone, "" },

        // ── String concatenation vs template literals ─────────────────────────
        { std::regex(R"(\+\s*\"|\"+\s*\+)"),
          "Consider using template literals instead of string concatenation",
          IssueSeverity::kWarning, FixKind::kNone, "" },
    };
    return kRules;
}

}  // namespace

AnalysisResult FixCodeAnalyzer::Analyze(const std::string& code) {
    AnalysisResult result;
    const auto& rules = Rules();

    std::istringstream stream(code);
    std::string line;
    int line_no = 0;

    while (std::getline(stream, line)) {
        ++line_no;

        // Skip single-line comments.
        const auto comment_pos = line.find("//");
        const std::string effective =
            (comment_pos != std::string::npos) ? line.substr(0, comment_pos) : line;

        for (const auto& rule : rules) {
            std::smatch match;
            if (std::regex_search(effective, match, rule.pattern)) {
                AnalysisIssue issue;
                issue.message      = rule.message;
                issue.line         = line_no;
                issue.column       = static_cast<int>(match.position()) + 1;
                issue.severity     = rule.severity;
                issue.fix_kind     = rule.fix_kind;
                issue.match_text   = match.str();
                if (rule.fix_kind == FixKind::kAutoReplace) {
                    issue.replacement_text = rule.replacement;
                }
                result.issues.push_back(std::move(issue));
                if (rule.severity == IssueSeverity::kError) {
                    result.has_errors = true;
                }
            }
        }
    }

    return result;
}

} // namespace AutoCorrection
