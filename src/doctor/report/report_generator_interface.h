/**
 * @file report_generator_interface.h
 * @brief Report generation interface.
 */

#pragma once

#include "core/engine_doctor_config.h"

namespace EngineDoctor {

class ReportGeneratorInterface {
public:
	virtual ~ReportGeneratorInterface() = default;

	virtual void initialize(const Config&) {}
	virtual void generate() = 0;
};

} // namespace EngineDoctor
