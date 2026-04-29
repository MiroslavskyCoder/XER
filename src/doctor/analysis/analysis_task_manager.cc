/**
 * @file analysis_task_manager.cc
 * @brief AnalysisTaskManager implementation.
 */

#include "analysis/analysis_task_manager.h"

#include <algorithm>
#include <unordered_map>

namespace EngineDoctor {

AnalysisTaskManager::AnalysisTaskManager(Context& context)
	: context_(context) {}

std::vector<AnalysisTaskSummary> AnalysisTaskManager::build_task_summaries(
	const std::vector<ScanResult>& scan_results,
	const AnalysisMetrics& metrics) const {
	size_t files_with_dependencies = 0;
	for (const auto& result : scan_results) {
		if (!result.dependencies.empty()) {
			++files_with_dependencies;
		}
	}

	std::vector<AnalysisTaskSummary> tasks;
	tasks.push_back({
		"scan_health",
		metrics.failed_files + metrics.files_with_warnings,
		metrics.total_errors + metrics.total_warnings,
		"Summarizes file-level errors and warnings collected by the scanner.",
	});
	tasks.push_back({
		"dependency_surface",
		files_with_dependencies,
		metrics.total_dependencies,
		"Tracks source files that declare includes or other lightweight dependencies.",
	});
	tasks.push_back({
		"empty_files",
		metrics.empty_files,
		metrics.empty_files,
		"Highlights zero-byte files that often indicate truncated assets or incomplete scaffolding.",
	});
	return tasks;
}

std::vector<std::string> AnalysisTaskManager::build_dominant_problem_codes(
	const std::vector<ScanResult>& scan_results,
	size_t limit) const {
	std::unordered_map<std::string, size_t> counters;
	for (const auto& result : scan_results) {
		for (const auto& error : result.errors) {
			++counters[error.code];
		}
		for (const auto& warning : result.warnings) {
			++counters[warning.code];
		}
	}

	std::vector<std::pair<std::string, size_t>> ordered(counters.begin(), counters.end());
	std::sort(ordered.begin(), ordered.end(), [](const auto& lhs, const auto& rhs) {
		if (lhs.second != rhs.second) {
			return lhs.second > rhs.second;
		}
		return lhs.first < rhs.first;
	});

	if (ordered.size() > limit) {
		ordered.resize(limit);
	}

	std::vector<std::string> codes;
	codes.reserve(ordered.size());
	for (const auto& entry : ordered) {
		codes.push_back(entry.first);
	}
	return codes;
}

} // namespace EngineDoctor
