#pragma once

#include "fix_code_analyzer.h"
#include <string>
#include <vector>

namespace AutoCorrection {

struct FixSuggestion {
    int id = 0;
    std::string description;
    std::string code_diff;
};

class FixSuggestionProvider {
public:
    std::vector<FixSuggestion> GetSuggestions(const AnalysisResult& result);
};

} // namespace AutoCorrection
