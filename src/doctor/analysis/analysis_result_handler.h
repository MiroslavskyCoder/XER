/**
 * @file analysis_result_handler.h
 * @brief Publishing and formatting of analysis results.
 */

#pragma once

#include "core/engine_doctor_context.h"

#include <cstddef>
#include <string>
#include <vector>

namespace EngineDoctor {

struct AnalysisMetrics {
	size_t total_files = 0;
	size_t successful_files = 0;
	size_t files_with_warnings = 0;
	size_t failed_files = 0;
	size_t total_errors = 0;
	size_t total_warnings = 0;
	size_t total_dependencies = 0;
	size_t empty_files = 0;
};

struct AnalysisTaskSummary {
	std::string task_name;
	size_t affected_files = 0;
	size_t issue_count = 0;
	std::string description;
};

struct AnalysisSummary {
	AnalysisMetrics metrics;
	std::vector<AnalysisTaskSummary> task_summaries;
	std::vector<std::string> dominant_problem_codes;
	std::string generated_at_utc;
};

class AnalysisResultHandler {
public:
	explicit AnalysisResultHandler(Context& context);

	void publish(AnalysisSummary summary);
	std::string build_text_report(const AnalysisSummary& summary) const;

private:
	Context& context_;
};

} // namespace EngineDoctor
