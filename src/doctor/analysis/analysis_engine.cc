/**
 * @file analysis_engine.cc
 * @brief AnalysisEngine implementation.
 */

#include "analysis/analysis_engine.h"

#include "core/logger.h"
#include "scanner/scan_result.h"

#include <vector>

namespace EngineDoctor {

AnalysisEngine::AnalysisEngine(Context& context)
	: context_(context),
	  data_provider_(context),
	  pipeline_(context),
	  result_handler_(context),
	  strategy_(CreateAnalysisStrategy("FullCodeAnalysis")),
	  config_(NormalizeConfig(Config{})) {}

void AnalysisEngine::initialize(const Config& config) {
	config_ = NormalizeConfig(config);
	strategy_ = CreateAnalysisStrategy(config_.analysis_strategy);
	context_.set_data(kContextConfigDataKey, config_);
	context_.set_data("analysis_strategy_name", strategy_->name());
	Logger::initialize(config_.log_level);
	Logger::debug("AnalysisEngine: initialized with strategy %s.", strategy_->name().c_str());
	if (config_.publish_events) {
		context_.event_bus().publish(
			"doctor.analysis.initialized",
			"AnalysisEngine initialized",
			{{"strategy", strategy_->name()}, {"config", BuildConfigSummary(config_)}});
	}
}

void AnalysisEngine::analyze() {
	const Config resolved_config = data_provider_.resolved_config();
	if (!strategy_) {
		strategy_ = CreateAnalysisStrategy(resolved_config.analysis_strategy);
	}
	context_.set_data("analysis_strategy_name", strategy_->name());

	const auto* scan_results = data_provider_.scan_results();
	const size_t scan_result_count = scan_results == nullptr ? 0u : scan_results->size();
	if (resolved_config.publish_events) {
		context_.event_bus().publish(
			"doctor.analysis.started",
			"AnalysisEngine analyze started",
			{{"scan_result_count", std::to_string(scan_result_count)}, {"strategy", strategy_->name()}});
	}

	if (scan_results == nullptr) {
		Logger::warning("AnalysisEngine: no scan results available, publishing empty summary.");
		const AnalysisSummary summary = strategy_->execute({}, pipeline_);
		result_handler_.publish(summary);
		if (resolved_config.publish_events) {
			context_.event_bus().publish(
				"doctor.analysis.completed",
				"AnalysisEngine analysis completed without scan results",
				{{"scan_result_count", "0"}, {"strategy", strategy_->name()}});
		}
		return;
	}

	Logger::info("AnalysisEngine: analyzing %zu scan results using %s.", scan_results->size(), strategy_->name().c_str());
	const AnalysisSummary summary = strategy_->execute(*scan_results, pipeline_);
	result_handler_.publish(summary);
	if (resolved_config.publish_events) {
		context_.event_bus().publish(
			"doctor.analysis.completed",
			"AnalysisEngine analysis completed",
			{{"scan_result_count", std::to_string(scan_results->size())},
			 {"strategy", strategy_->name()},
			 {"dominant_problem_code_count", std::to_string(summary.dominant_problem_codes.size())}});
	}
}

} // namespace EngineDoctor
