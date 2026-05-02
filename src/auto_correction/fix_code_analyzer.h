#pragma once

#include <string>
#include <vector>

namespace AutoCorrection {

struct AnalysisIssue {
    std::string message;
    int line = 0;
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
