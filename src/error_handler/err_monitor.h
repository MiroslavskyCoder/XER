#pragma once

#include <string>
#include <string_view>

namespace Engine::ErrorHandler {

struct MonitorConfiguration {
	bool verbose = false;
	bool timestamps = false;
	std::string log_level;
};

void InitializeMonitor(const MonitorConfiguration& config);
void ReportStartupEvent(std::string_view component, std::string_view message);
void ReportStartupError(std::string_view component, std::string_view message);

}  // namespace Engine::ErrorHandler
