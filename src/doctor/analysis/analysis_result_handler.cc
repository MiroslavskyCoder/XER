/**
 * @file analysis_result_handler.cc
 * @brief Analysis result publishing.
 */

#include "analysis/analysis_result_handler.h"

#include "analysis/data_provider.h"

#include <sstream>

namespace EngineDoctor {

AnalysisResultHandler::AnalysisResultHandler(Context& context)
	: context_(context) {}

void AnalysisResultHandler::publish(AnalysisSummary summary) {
	DataProvider data_provider(context_);
	const Config config = data_provider.resolved_config();
	const std::string report_text = build_text_report(summary);
	context_.set_data(config.analysis_summary_key, summary);
	context_.set_data(config.analysis_summary_text_key, report_text);
	if (config.publish_events) {
		context_.event_bus().publish(
			"doctor.analysis.summary.published",
			"Analysis summary published",
			{{"summary_key", config.analysis_summary_key},
			 {"summary_text_key", config.analysis_summary_text_key},
			 {"generated_at_utc", summary.generated_at_utc}});
	}
}

std::string AnalysisResultHandler::build_text_report(const AnalysisSummary& summary) const {
	std::ostringstream output;
	output << "EngineDoctor Analysis Summary\n";
	if (const auto* strategy_name = context_.get_data<std::string>("analysis_strategy_name")) {
		output << "analysis_strategy=" << *strategy_name << "\n";
	}
	output << "generated_at_utc=" << summary.generated_at_utc << "\n";
	output << "total_files=" << summary.metrics.total_files << "\n";
	output << "successful_files=" << summary.metrics.successful_files << "\n";
	output << "files_with_warnings=" << summary.metrics.files_with_warnings << "\n";
	output << "failed_files=" << summary.metrics.failed_files << "\n";
	output << "total_errors=" << summary.metrics.total_errors << "\n";
	output << "total_warnings=" << summary.metrics.total_warnings << "\n";
	output << "total_dependencies=" << summary.metrics.total_dependencies << "\n";
	output << "empty_files=" << summary.metrics.empty_files << "\n";

	for (size_t index = 0; index < summary.task_summaries.size(); ++index) {
		const auto& task = summary.task_summaries[index];
		output << "task_" << index << "_name=" << task.task_name << "\n";
		output << "task_" << index << "_affected_files=" << task.affected_files << "\n";
		output << "task_" << index << "_issue_count=" << task.issue_count << "\n";
		output << "task_" << index << "_description=" << task.description << "\n";
	}

	for (size_t index = 0; index < summary.dominant_problem_codes.size(); ++index) {
		output << "problem_code_" << index << "=" << summary.dominant_problem_codes[index] << "\n";
	}

	return output.str();
}

} // namespace EngineDoctor
