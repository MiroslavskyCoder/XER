/**
 * @file analysis_pipeline.h
 * @brief Analysis orchestration over collected scan results.
 */

#pragma once

#include "analysis/analysis_result_handler.h"
#include "analysis/analysis_task_manager.h"
#include "analysis/metric_calculator.h"
#include "core/engine_doctor_context.h"
#include "scanner/scan_result.h"

#include <vector>

namespace EngineDoctor {

class AnalysisPipeline {
public:
	explicit AnalysisPipeline(Context& context);

	AnalysisSummary run(const std::vector<ScanResult>& scan_results);

private:
	Context& context_;
	MetricCalculator metric_calculator_;
	AnalysisTaskManager task_manager_;
};

} // namespace EngineDoctor
