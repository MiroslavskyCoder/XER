/**
 * @file scan_parameters.h
 * @brief Scan-time configuration for ScannerModule.
 */

#pragma once

#include <string>
#include <vector>

namespace EngineDoctor {

struct ScanParameters {
	std::vector<std::string> paths_to_scan;
	bool analyze_syntax = true;
	bool analyze_dependencies = true;
	bool analyze_security = false;
	bool include_hidden = false;
};

} // namespace EngineDoctor
