/**
 * @file engine_doctor_config.h
 * @brief Runtime configuration for EngineDoctor.
 */

#pragma once

#include <string>

namespace EngineDoctor {

struct Config {
	int log_level = 1;
	std::string report_output_path;
};

} // namespace EngineDoctor
