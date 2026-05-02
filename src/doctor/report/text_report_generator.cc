#include "report/text_report_generator.h"
namespace EngineDoctor {
std::string TextReportGenerator::Generate(const HealthReport& report) {
    std::string out;
    out += "=== Health Report ===\n";
    out += "Status: ";
    switch (report.status) {
        case HealthStatus::OK: out += "OK\n"; break;
        case HealthStatus::DEGRADED: out += "DEGRADED\n"; break;
        case HealthStatus::CRITICAL: out += "CRITICAL\n"; break;
    }
    for (const auto& issue : report.issues) out += "  - " + issue + "\n";
    return out;
}
}
