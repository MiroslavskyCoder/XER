#pragma once
#include <string>
#include "diagnostics/health_checker.h"
namespace EngineDoctor {
class TextReportGenerator {
public:
    std::string Generate(const HealthReport& report);
};
}
