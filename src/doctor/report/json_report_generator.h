#pragma once
#include <string>
#include "diagnostics/health_checker.h"
namespace EngineDoctor {
class JsonReportGenerator {
public:
    std::string Generate(const HealthReport& report);
};
}
