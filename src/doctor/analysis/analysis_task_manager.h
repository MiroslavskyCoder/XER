/**
 * @file analysis_task_manager.h
 * @brief Builds higher-level analysis tasks from scan output.
 */

#pragma once

#include "analysis/analysis_result_handler.h"
#include "scanner/scan_result.h"
#include "core/engine_doctor_context.h"

#include <vector>

namespace EngineDoctor {

class AnalysisTaskManager {
public:
	explicit AnalysisTaskManager(Context& context);

	std::vector<AnalysisTaskSummary> build_task_summaries(
		const std::vector<ScanResult>& scan_results,
		const AnalysisMetrics& metrics) const;

	std::vector<std::string> build_dominant_problem_codes(
		const std::vector<ScanResult>& scan_results,
		size_t limit = 5) const;

private:
	Context& context_;
};

} // namespace EngineDoctor
