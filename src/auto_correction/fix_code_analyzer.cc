#include "fix_code_analyzer.h"

#include <sstream>
#include <regex>

namespace AutoCorrection {

AnalysisResult FixCodeAnalyzer::Analyze(const std::string& code) {
    AnalysisResult result;

    static const std::vector<std::pair<std::regex, std::string>> patterns = {
        { std::regex(R"(eval\s*\()"),          "Use of eval() is discouraged" },
        { std::regex(R"(==\s)"),               "Prefer === over ==" },
        { std::regex(R"(var\s+)"),             "Prefer let/const over var" },
        { std::regex(R"(console\.log\s*\()"),  "Remove debug console.log" },
    };

    std::istringstream stream(code);
    std::string line;
    int line_no = 0;

    while (std::getline(stream, line)) {
        ++line_no;
        for (const auto& [pattern, message] : patterns) {
            if (std::regex_search(line, pattern)) {
                result.issues.push_back({ message, line_no });
                result.has_errors = true;
            }
        }
    }

    return result;
}

} // namespace AutoCorrection
