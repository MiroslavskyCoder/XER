#include "custom_effect_struct.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace Engine::Audio::FX {

namespace {

std::string NormalizeText(std::string text) {
	std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});
	return text;
}

}  // namespace

std::string CustomEffectBackendToString(CustomEffectBackend backend) {
	switch (backend) {
	case CustomEffectBackend::kBuiltinAlgorithm:
		return "builtin";
	case CustomEffectBackend::kClapPlugin:
		return "clap";
	}
	return "builtin";
}

bool ParseCustomEffectBackend(const std::string& text, CustomEffectBackend* backend_out) {
	if (backend_out == nullptr) {
		return false;
	}
	const std::string normalized = NormalizeText(text);
	if (normalized == "builtin" || normalized == "algorithm") {
		*backend_out = CustomEffectBackend::kBuiltinAlgorithm;
		return true;
	}
	if (normalized == "clap" || normalized == "plugin") {
		*backend_out = CustomEffectBackend::kClapPlugin;
		return true;
	}
	return false;
}

std::string CustomEffectAlgorithmToString(CustomEffectAlgorithm algorithm) {
	switch (algorithm) {
	case CustomEffectAlgorithm::kPassthrough:
		return "passthrough";
	case CustomEffectAlgorithm::kCompressor:
		return "compressor";
	case CustomEffectAlgorithm::kChorus:
		return "chorus";
	case CustomEffectAlgorithm::kParametricEq:
		return "parametric_eq";
	case CustomEffectAlgorithm::kLimiter:
		return "limiter";
	case CustomEffectAlgorithm::kPlugin:
		return "plugin";
	case CustomEffectAlgorithm::kUnknown:
		break;
	}
	return "unknown";
}

bool ParseCustomEffectAlgorithm(const std::string& text, CustomEffectAlgorithm* algorithm_out) {
	if (algorithm_out == nullptr) {
		return false;
	}
	const std::string normalized = NormalizeText(text);
	if (normalized == "passthrough") {
		*algorithm_out = CustomEffectAlgorithm::kPassthrough;
		return true;
	}
	if (normalized == "compressor") {
		*algorithm_out = CustomEffectAlgorithm::kCompressor;
		return true;
	}
	if (normalized == "chorus") {
		*algorithm_out = CustomEffectAlgorithm::kChorus;
		return true;
	}
	if (normalized == "parametric_eq" || normalized == "eq") {
		*algorithm_out = CustomEffectAlgorithm::kParametricEq;
		return true;
	}
	if (normalized == "limiter") {
		*algorithm_out = CustomEffectAlgorithm::kLimiter;
		return true;
	}
	if (normalized == "plugin" || normalized == "clap_plugin") {
		*algorithm_out = CustomEffectAlgorithm::kPlugin;
		return true;
	}
	*algorithm_out = CustomEffectAlgorithm::kUnknown;
	return false;
}

std::string BuildCustomEffectReportText(const CustomEffectReport& report) {
	std::ostringstream output;
	output << std::fixed << std::setprecision(6);
	output << "CustomEffectReport\n";
	output << "label=" << report.label << "\n";
	output << "node_count=" << report.node_count << "\n";
	output << "stage_count=" << report.stage_count << "\n";
	output << "worker_count_used=" << report.worker_count_used << "\n";
	output << "peak=" << report.peak << "\n";
	output << "rms=" << report.rms << "\n";
	for (size_t index = 0; index < report.stage_reports.size(); ++index) {
		output << "stage_report_" << index << "=" << report.stage_reports[index] << "\n";
	}
	for (size_t index = 0; index < report.node_reports.size(); ++index) {
		output << "node_report_" << index << "=" << report.node_reports[index] << "\n";
	}
	return output.str();
}

}  // namespace Engine::Audio::FX
