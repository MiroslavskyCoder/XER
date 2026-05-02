#include "fix_engine.h"

#include <algorithm>

namespace AutoCorrection {

bool FixEngine::Run(FixContext& ctx) {
    current_ctx_ = &ctx;
    applied_fixes_.clear();
    return !ctx.source_code.empty();
}

bool FixEngine::ApplyFix(int fix_id) {
    if (!current_ctx_) return false;
    applied_fixes_.push_back(fix_id);
    return true;
}

void FixEngine::Rollback() {
    if (applied_fixes_.empty()) return;
    applied_fixes_.pop_back();
}

} // namespace AutoCorrection
