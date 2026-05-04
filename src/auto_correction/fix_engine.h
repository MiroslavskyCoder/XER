#pragma once

#include <string>
#include <vector>

#include "fix_code_analyzer.h"

namespace AutoCorrection {

struct FixContext {
    std::string file_path;
    std::string source_code;
};

class FixEngine {
public:
    // Run analyzer on ctx.source_code. Populates last_result_.
    // Does not modify source_code; call ApplyFix() for that.
    // Returns true if analysis succeeded (even with issues).
    bool Run(FixContext& ctx);

    // Apply the auto-fix for issue at |issue_index| in last_result_.
    // Modifies ctx.source_code in-place and records the change for Rollback().
    // Returns false if the issue has no automatic fix or index is out of range.
    bool ApplyFix(int issue_index);

    // Undo the last applied fix.
    void Rollback();

    const AnalysisResult& LastResult() const { return last_result_; }

private:
    struct AppliedFix {
        int   issue_index;
        std::string original_source;  // full source before the fix
    };

    AnalysisResult     last_result_;
    std::vector<AppliedFix> applied_fixes_;
    FixContext*         current_ctx_ = nullptr;
};

} // namespace AutoCorrection
