#include "analysis/analysis_strategy.h"

#include "core/engine_doctor_config.h"

#include <memory>
#include <string>

namespace EngineDoctor {
namespace {

std::string NormalizeStrategyName(std::string_view strategy_name) {
	if (!IsKnownAnalysisStrategy(strategy_name)) {
		return "FullCodeAnalysis";
	}
	std::string normalized(strategy_name);
	if (normalized == "FullCodeAnalysis" || normalized == "DependencyCheckStrategy"
		|| normalized == "SecurityAuditStrategy" || normalized == "PerformanceAnalysisStrategy"
		|| normalized == "V8CompatibilityStrategy") {
		return normalized;
	}
	std::string compact;
	compact.reserve(normalized.size());
	for (char ch : normalized) {
		if (ch != ' ' && ch != '_' && ch != '-') {
			compact.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
		}
	}
	if (compact == "dependencycheckstrategy") {
		return "DependencyCheckStrategy";
	}
	if (compact == "securityauditstrategy") {
		return "SecurityAuditStrategy";
	}
	if (compact == "performanceanalysisstrategy") {
		return "PerformanceAnalysisStrategy";
	}
	if (compact == "v8compatibilitystrategy") {
		return "V8CompatibilityStrategy";
	}
	return "FullCodeAnalysis";
}

AnalysisTaskSummary BuildStrategyTask(const std::string& strategy_name,
						 const AnalysisSummary& summary,
						 std::size_t scan_result_count) {
	AnalysisTaskSummary task;
	task.task_name = "strategy_profile";
	task.affected_files = scan_result_count;
	task.issue_count = summary.metrics.total_errors + summary.metrics.total_warnings;
	task.description = "Analysis executed using strategy profile '" + strategy_name
		+ "' on top of the shared EngineDoctor analysis pipeline.";
	if (strategy_name == "DependencyCheckStrategy") {
		task.issue_count = summary.metrics.total_dependencies;
		task.description = "Emphasizes dependency edges discovered by the scanner and the files that declare them.";
	} else if (strategy_name == "SecurityAuditStrategy") {
		task.description = "Highlights warning and error density as a coarse security-oriented audit signal until dedicated security analyzers land.";
	} else if (strategy_name == "PerformanceAnalysisStrategy") {
		task.issue_count = summary.metrics.files_with_warnings + summary.metrics.failed_files;
		task.description = "Reuses the shared pipeline and flags files that may need follow-up performance-oriented review.";
	} else if (strategy_name == "V8CompatibilityStrategy") {
		task.issue_count = summary.metrics.total_dependencies;
		task.description = "Runs the shared pipeline with a V8-focused strategy label so compatibility-specific analyzers can plug in later.";
	}
	return task;
}

class NamedPipelineStrategy final : public AnalysisStrategy {
public:
	explicit NamedPipelineStrategy(std::string strategy_name)
		: strategy_name_(std::move(strategy_name)) {}

	std::string name() const override {
		return strategy_name_;
	}

	AnalysisSummary execute(
		const std::vector<ScanResult>& scan_results,
		AnalysisPipeline& pipeline) const override {
		AnalysisSummary summary = pipeline.run(scan_results);
		summary.task_summaries.push_back(BuildStrategyTask(strategy_name_, summary, scan_results.size()));
		return summary;
	}

private:
	std::string strategy_name_;
};

} // namespace

std::unique_ptr<AnalysisStrategy> CreateAnalysisStrategy(std::string_view strategy_name) {
	return std::make_unique<NamedPipelineStrategy>(NormalizeStrategyName(strategy_name));
}

} // namespace EngineDoctor
