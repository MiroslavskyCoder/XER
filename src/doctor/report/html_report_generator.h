#pragma once
#include <string>
#include "diagnostics/health_checker.h"
namespace EngineDoctor {
class HtmlReportGenerator {
public:
    std::string Generate(const HealthReport& report);
};
}
