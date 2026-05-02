#include "report/json_report_generator.h"
namespace EngineDoctor {
std::string JsonReportGenerator::Generate(const HealthReport& report) {
    std::string status;
    switch (report.status) {
        case HealthStatus::OK: status = "ok"; break;
        case HealthStatus::DEGRADED: status = "degraded"; break;
        case HealthStatus::CRITICAL: status = "critical"; break;
    }
    std::string out = "{\"status\":\"" + status + "\",\"issues\":[";
    for (size_t i = 0; i < report.issues.size(); ++i) {
        if (i) out += ",";
        out += "\"" + report.issues[i] + "\"";
    }
    out += "]}";
    return out;
}
}
