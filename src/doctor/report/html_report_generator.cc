#include "report/html_report_generator.h"
namespace EngineDoctor {
std::string HtmlReportGenerator::Generate(const HealthReport& report) {
    std::string status;
    std::string color;
    switch (report.status) {
        case HealthStatus::OK: status="OK"; color="#2ecc71"; break;
        case HealthStatus::DEGRADED: status="DEGRADED"; color="#f39c12"; break;
        case HealthStatus::CRITICAL: status="CRITICAL"; color="#e74c3c"; break;
    }
    std::string out = "<html><body>";
    out += "<h1 style=\"color:" + color + "\">Status: " + status + "</h1><ul>";
    for (const auto& issue : report.issues) out += "<li>" + issue + "</li>";
    out += "</ul></body></html>";
    return out;
}
}
