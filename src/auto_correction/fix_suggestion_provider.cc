#include "fix_suggestion_provider.h"

namespace AutoCorrection {

std::vector<FixSuggestion> FixSuggestionProvider::GetSuggestions(const AnalysisResult& result) {
    std::vector<FixSuggestion> suggestions;
    int id = 1;

    for (const auto& issue : result.issues) {
        FixSuggestion s;
        s.id = id++;
        s.description = "Line " + std::to_string(issue.line) + ": " + issue.message;

        if (issue.message.find("eval") != std::string::npos) {
            s.code_diff = "-eval(...)\n+// Replace eval with a safe alternative";
        } else if (issue.message.find("===") != std::string::npos) {
            s.code_diff = "-a == b\n+a === b";
        } else if (issue.message.find("var") != std::string::npos) {
            s.code_diff = "-var x\n+const x";
        } else if (issue.message.find("console.log") != std::string::npos) {
            s.code_diff = "-console.log(...)\n+// Remove debug output";
        }

        suggestions.push_back(std::move(s));
    }

    return suggestions;
}

} // namespace AutoCorrection
