#pragma once
#include <string>
#include <vector>
#include "diagnostics/health_checker.h"
namespace EngineDoctor {
class ReportDataFormatter {
public:
    static std::string FormatIssueList(const std::vector<std::string>& issues);
    static std::string FormatStatus(HealthStatus status);
};
}
