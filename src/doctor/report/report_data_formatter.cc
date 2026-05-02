#include "report/report_data_formatter.h"
namespace EngineDoctor {
std::string ReportDataFormatter::FormatIssueList(const std::vector<std::string>& issues) {
    std::string out;
    for (const auto& i : issues) out += "- " + i + "\n";
    return out;
}
std::string ReportDataFormatter::FormatStatus(HealthStatus status) {
    switch (status) {
        case HealthStatus::OK: return "OK";
        case HealthStatus::DEGRADED: return "DEGRADED";
        case HealthStatus::CRITICAL: return "CRITICAL";
    }
    return "UNKNOWN";
}
}
