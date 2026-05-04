#include "fix_engine.h"

#include <regex>
#include <sstream>
#include <string>

namespace AutoCorrection {

bool FixEngine::Run(FixContext& ctx) {
    if (ctx.source_code.empty()) {
        return false;
    }
    current_ctx_ = &ctx;
    applied_fixes_.clear();
    FixCodeAnalyzer analyzer;
    last_result_ = analyzer.Analyze(ctx.source_code);
    return true;
}

bool FixEngine::ApplyFix(int issue_index) {
    if (!current_ctx_) {
        return false;
    }
    const int count = static_cast<int>(last_result_.issues.size());
    if (issue_index < 0 || issue_index >= count) {
        return false;
    }
    const AnalysisIssue& issue = last_result_.issues[static_cast<size_t>(issue_index)];
    if (issue.fix_kind == FixKind::kNone) {
        return false;
    }

    // Save original for rollback.
    applied_fixes_.push_back({issue_index, current_ctx_->source_code});

    if (issue.fix_kind == FixKind::kAutoReplace) {
        // Replace the first occurrence of match_text on the target line.
        std::istringstream in(current_ctx_->source_code);
        std::ostringstream out;
        std::string line;
        int line_no = 0;
        bool fixed = false;
        while (std::getline(in, line)) {
            ++line_no;
            if (!fixed && line_no == issue.line && !issue.match_text.empty()) {
                const auto pos = line.find(issue.match_text);
                if (pos != std::string::npos) {
                    line.replace(pos, issue.match_text.size(), issue.replacement_text);
                    fixed = true;
                }
            }
            out << line << '\n';
        }
        current_ctx_->source_code = out.str();
        // Trim trailing newline added by the loop if the original didn't have one.
        if (!current_ctx_->source_code.empty() &&
            current_ctx_->source_code.back() == '\n') {
            const bool had_trailing_nl =
                !applied_fixes_.back().original_source.empty() &&
                applied_fixes_.back().original_source.back() == '\n';
            if (!had_trailing_nl) {
                current_ctx_->source_code.pop_back();
            }
        }
        return fixed;
    }

    if (issue.fix_kind == FixKind::kAutoRemove) {
        // Remove the entire line.
        std::istringstream in(current_ctx_->source_code);
        std::ostringstream out;
        std::string line;
        int line_no = 0;
        while (std::getline(in, line)) {
            ++line_no;
            if (line_no != issue.line) {
                out << line << '\n';
            }
        }
        current_ctx_->source_code = out.str();
        return true;
    }

    return false;
}

void FixEngine::Rollback() {
    if (applied_fixes_.empty() || !current_ctx_) {
        return;
    }
    current_ctx_->source_code = applied_fixes_.back().original_source;
    applied_fixes_.pop_back();
}

} // namespace AutoCorrection
