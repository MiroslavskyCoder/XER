#pragma once

#include <string>
#include <vector>

namespace AutoCorrection {

enum class IssueSeverity {
    kWarning,
    kError,
};

// Whether the issue has a safe automatic fix.
enum class FixKind {
    kNone,        // Manual fix required.
    kAutoReplace, // Can be fixed by simple text substitution.
    kAutoRemove,  // The whole line can be removed.
};

struct AnalysisIssue {
    std::string  message;
    int          line      = 0;
    int          column    = 0;  // 1-based, 0 = unknown
    IssueSeverity severity = IssueSeverity::kWarning;
    FixKind      fix_kind  = FixKind::kNone;
    // For kAutoReplace: text to match and replacement.
    std::string  match_text;
    std::string  replacement_text;
};

struct AnalysisResult {
    std::vector<AnalysisIssue> issues;
    bool has_errors = false;
};

class FixCodeAnalyzer {
public:
    AnalysisResult Analyze(const std::string& code);
};

} // namespace AutoCorrection
