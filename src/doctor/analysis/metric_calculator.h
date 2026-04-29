/**
 * @file metric_calculator.h
 * @brief Aggregates low-level scan metrics.
 */

#pragma once

#include "analysis/analysis_result_handler.h"
#include "scanner/scan_result.h"

#include <vector>

namespace EngineDoctor {

class MetricCalculator {
public:
	AnalysisMetrics calculate(const std::vector<ScanResult>& scan_results) const;
};

} // namespace EngineDoctor
