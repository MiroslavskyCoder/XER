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
	  pipeline_(context),
	  result_handler_(context) {}

void AnalysisEngine::initialize(const Config& config) {
	Logger::initialize(config.log_level);
	Logger::debug("AnalysisEngine: initialized.");
}

void AnalysisEngine::analyze() {
	const auto* scan_results = context_.get_data<std::vector<ScanResult>>("scan_results");
	if (scan_results == nullptr) {
		Logger::warning("AnalysisEngine: no scan results available, publishing empty summary.");
		result_handler_.publish(pipeline_.run({}));
		return;
	}

	Logger::info("AnalysisEngine: analyzing %zu scan results.", scan_results->size());
	result_handler_.publish(pipeline_.run(*scan_results));
}

} // namespace EngineDoctor
