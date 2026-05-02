#pragma once

#include <string>
#include <vector>

namespace AutoCorrection {

struct FixContext {
    std::string file_path;
    std::string source_code;
};

class FixEngine {
public:
    bool Run(FixContext& ctx);
    bool ApplyFix(int fix_id);
    void Rollback();

private:
    std::vector<int> applied_fixes_;
    FixContext* current_ctx_ = nullptr;
};

} // namespace AutoCorrection
