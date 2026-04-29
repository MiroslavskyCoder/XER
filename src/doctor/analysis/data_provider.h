#pragma once

#include "core/engine_doctor_config.h"
#include "core/engine_doctor_context.h"
#include "scanner/scan_parameters.h"
#include "scanner/scan_result.h"

#include <vector>

namespace EngineDoctor {

class DataProvider {
public:
	explicit DataProvider(Context& context);

	Config resolved_config() const;
	const ScanParameters* scan_parameters() const;
	const std::vector<ScanResult>* scan_results() const;
	std::vector<ScanResult> scan_results_copy() const;
	bool has_scan_results() const;

private:
	Context& context_;
};

} // namespace EngineDoctor
