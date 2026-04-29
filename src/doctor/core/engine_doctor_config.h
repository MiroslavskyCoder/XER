/**
 * @file engine_doctor_config.h
 * @brief Runtime configuration for EngineDoctor.
 */

#pragma once

#include <string>
#include <string_view>

namespace EngineDoctor {

inline constexpr const char kContextConfigDataKey[] = "engine_doctor_config";
inline constexpr const char kDefaultScanParametersKey[] = "scan_parameters";
inline constexpr const char kDefaultScanResultsKey[] = "scan_results";
inline constexpr const char kDefaultAnalysisSummaryKey[] = "analysis_summary";
inline constexpr const char kDefaultAnalysisSummaryTextKey[] = "analysis_summary_text";

struct Config {
	int log_level = 1;
	std::string report_output_path;
	std::string analysis_strategy = "FullCodeAnalysis";
	std::string scan_parameters_key = kDefaultScanParametersKey;
	std::string scan_results_key = kDefaultScanResultsKey;
	std::string analysis_summary_key = kDefaultAnalysisSummaryKey;
	std::string analysis_summary_text_key = kDefaultAnalysisSummaryTextKey;
	bool publish_events = true;
};

bool IsKnownAnalysisStrategy(std::string_view strategy_name);
Config NormalizeConfig(const Config& config);
Config ConfigFromEnvironment();
std::string BuildConfigSummary(const Config& config);

} // namespace EngineDoctor
