#include "core/engine_doctor_config.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>

namespace EngineDoctor {
namespace {

std::string ToLower(std::string value) {
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});
	return value;
}

std::string EnvString(const char* key, const std::string& fallback = {}) {
	const char* raw = std::getenv(key);
	if (raw == nullptr || raw[0] == '\0') {
		return fallback;
	}
	return std::string(raw);
}

bool EnvBool(const char* key, bool fallback) {
	const std::string raw = ToLower(EnvString(key));
	if (raw.empty()) {
		return fallback;
	}
	return raw == "1" || raw == "true" || raw == "on" || raw == "yes";
}

int EnvInt(const char* key, int fallback) {
	const std::string raw = EnvString(key);
	if (raw.empty()) {
		return fallback;
	}
	try {
		return std::stoi(raw);
	} catch (...) {
		return fallback;
	}
}

std::string DefaultIfEmpty(std::string value, const char* fallback) {
	return value.empty() ? std::string(fallback) : value;
}

std::string CanonicalStrategy(std::string_view strategy_name) {
	std::string token;
	token.reserve(strategy_name.size());
	for (char ch : strategy_name) {
		if (ch == ' ' || ch == '_' || ch == '-') {
			continue;
		}
		token.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
	}
	return token;
}

} // namespace

bool IsKnownAnalysisStrategy(std::string_view strategy_name) {
	const std::string canonical = CanonicalStrategy(strategy_name);
	return canonical == "fullcodeanalysis"
		|| canonical == "dependencycheckstrategy"
		|| canonical == "securityauditstrategy"
		|| canonical == "performanceanalysisstrategy"
		|| canonical == "v8compatibilitystrategy";
}

Config NormalizeConfig(const Config& config) {
	Config normalized = config;
	if (normalized.log_level < 0) {
		normalized.log_level = 0;
	}
	if (!IsKnownAnalysisStrategy(normalized.analysis_strategy)) {
		normalized.analysis_strategy = "FullCodeAnalysis";
	}
	normalized.scan_parameters_key = DefaultIfEmpty(normalized.scan_parameters_key, kDefaultScanParametersKey);
	normalized.scan_results_key = DefaultIfEmpty(normalized.scan_results_key, kDefaultScanResultsKey);
	normalized.analysis_summary_key = DefaultIfEmpty(normalized.analysis_summary_key, kDefaultAnalysisSummaryKey);
	normalized.analysis_summary_text_key = DefaultIfEmpty(normalized.analysis_summary_text_key, kDefaultAnalysisSummaryTextKey);
	return normalized;
}

Config ConfigFromEnvironment() {
	Config config;
	config.log_level = EnvInt("ENGINE_DOCTOR_LOG_LEVEL", 1);
	config.report_output_path = EnvString("ENGINE_DOCTOR_REPORT_OUTPUT");
	config.analysis_strategy = EnvString("ENGINE_DOCTOR_ANALYSIS_STRATEGY", "FullCodeAnalysis");
	config.scan_parameters_key = EnvString("ENGINE_DOCTOR_SCAN_PARAMETERS_KEY", kDefaultScanParametersKey);
	config.scan_results_key = EnvString("ENGINE_DOCTOR_SCAN_RESULTS_KEY", kDefaultScanResultsKey);
	config.analysis_summary_key = EnvString("ENGINE_DOCTOR_ANALYSIS_SUMMARY_KEY", kDefaultAnalysisSummaryKey);
	config.analysis_summary_text_key = EnvString("ENGINE_DOCTOR_ANALYSIS_SUMMARY_TEXT_KEY", kDefaultAnalysisSummaryTextKey);
	config.publish_events = !EnvBool("ENGINE_DOCTOR_DISABLE_EVENTS", false);
	return NormalizeConfig(config);
}

std::string BuildConfigSummary(const Config& config) {
	const Config normalized = NormalizeConfig(config);
	std::ostringstream output;
	output << "log_level=" << normalized.log_level
		   << ", strategy=" << normalized.analysis_strategy
		   << ", scan_parameters_key=" << normalized.scan_parameters_key
		   << ", scan_results_key=" << normalized.scan_results_key
		   << ", analysis_summary_key=" << normalized.analysis_summary_key
		   << ", publish_events=" << (normalized.publish_events ? "true" : "false");
	return output.str();
}

} // namespace EngineDoctor
