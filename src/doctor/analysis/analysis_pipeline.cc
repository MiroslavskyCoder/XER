/**
 * @file analysis_pipeline.cc
 * @brief AnalysisPipeline implementation.
 */

#include "analysis/analysis_pipeline.h"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace EngineDoctor {
namespace {

std::string BuildUtcTimestamp() {
	const std::time_t now = std::time(nullptr);
	std::tm tm_now {};
#if defined(_WIN32)
	gmtime_s(&tm_now, &now);
#else
	gmtime_r(&now, &tm_now);
#endif
	std::ostringstream output;
	output << std::put_time(&tm_now, "%Y-%m-%dT%H:%M:%SZ");
	return output.str();
}

} // namespace

AnalysisPipeline::AnalysisPipeline(Context& context)
	: context_(context),
	  task_manager_(context) {}

AnalysisSummary AnalysisPipeline::run(const std::vector<ScanResult>& scan_results) {
	AnalysisSummary summary;
	summary.metrics = metric_calculator_.calculate(scan_results);
	summary.task_summaries = task_manager_.build_task_summaries(scan_results, summary.metrics);
	summary.dominant_problem_codes = task_manager_.build_dominant_problem_codes(scan_results);
	summary.generated_at_utc = BuildUtcTimestamp();
	return summary;
}

} // namespace EngineDoctor
