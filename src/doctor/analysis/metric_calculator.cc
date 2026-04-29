/**
 * @file metric_calculator.cc
 * @brief MetricCalculator implementation.
 */

#include "analysis/metric_calculator.h"

namespace EngineDoctor {

AnalysisMetrics MetricCalculator::calculate(const std::vector<ScanResult>& scan_results) const {
	AnalysisMetrics metrics;
	metrics.total_files = scan_results.size();

	for (const auto& result : scan_results) {
		switch (result.status) {
		case ScanStatus::SUCCESS:
			++metrics.successful_files;
			break;
		case ScanStatus::WARNING:
			++metrics.files_with_warnings;
			break;
		case ScanStatus::ERROR:
			++metrics.failed_files;
			break;
		case ScanStatus::PENDING:
		case ScanStatus::SKIPPED:
			break;
		}

		metrics.total_errors += result.errors.size();
		metrics.total_warnings += result.warnings.size();
		metrics.total_dependencies += result.dependencies.size();
		if (result.metadata.size == 0u) {
			++metrics.empty_files;
		}
	}

	return metrics;
}

} // namespace EngineDoctor
