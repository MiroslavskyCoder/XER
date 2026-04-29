/**
 * @file scanner_module.cc
 * @brief ScannerModule implementation.
 */

#include "scanner/scanner_module.h"

#include "core/logger.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <system_error>

namespace EngineDoctor {
namespace {

std::string ToLower(std::string value) {
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});
	return value;
}

void ParseSourceFile(
	const std::filesystem::path& path,
	bool collect_dependencies,
	bool check_braces,
	std::vector<std::string>* dependencies_out,
	std::vector<ScanMessage>* warnings_out) {
	std::ifstream input(path);
	if (!input.is_open()) {
		return;
	}

	int brace_balance = 0;
	std::string line;
	while (std::getline(input, line)) {
		if (collect_dependencies) {
			const std::string marker = "#include";
			const size_t include_position = line.find(marker);
			if (include_position != std::string::npos) {
				const size_t quote_begin = line.find_first_of("<\"", include_position + marker.size());
				const size_t quote_end = quote_begin == std::string::npos
					? std::string::npos
					: line.find_first_of(">\"", quote_begin + 1u);
				if (quote_begin != std::string::npos && quote_end != std::string::npos && quote_end > quote_begin + 1u) {
					dependencies_out->push_back(line.substr(quote_begin + 1u, quote_end - quote_begin - 1u));
				}
			}
		}

		if (check_braces) {
			for (char ch : line) {
				if (ch == '{') {
					++brace_balance;
				} else if (ch == '}') {
					--brace_balance;
				}
			}
		}
	}

	if (check_braces && brace_balance != 0) {
		warnings_out->emplace_back("UNBALANCED_BRACES", "Brace count is unbalanced in source file.");
	}
}

} // namespace

ScannerModule::ScannerModule(Context& context)
	: context_(context) {}

void ScannerModule::initialize(const Config& config) {
	Logger::initialize(config.log_level);
	Logger::debug("ScannerModule: initialized.");
}

void ScannerModule::scan(const ScanParameters& params) {
	std::vector<ScanResult> results;
	for (const auto& raw_path : params.paths_to_scan) {
		scan_path(std::filesystem::path(raw_path), params, &results);
	}

	context_.set_data("scan_parameters", params);
	context_.set_data("scan_results", results);
	Logger::info("ScannerModule: produced %zu scan results.", results.size());
}

void ScannerModule::scan_path(
	const std::filesystem::path& path,
	const ScanParameters& params,
	std::vector<ScanResult>* results) const {
	if (results == nullptr) {
		return;
	}

	std::error_code error;
	if (!std::filesystem::exists(path, error)) {
		ScanResult result;
		result.file_path = path.string();
		result.metadata.full_path = path.string();
		result.status = ScanStatus::ERROR;
		result.error_message = "Path does not exist.";
		result.errors.emplace_back("PATH_NOT_FOUND", result.error_message);
		results->push_back(std::move(result));
		return;
	}

	if (std::filesystem::is_regular_file(path, error)) {
		results->push_back(scan_file(path, params));
		return;
	}

	if (!std::filesystem::is_directory(path, error)) {
		ScanResult result;
		result.file_path = path.string();
		result.metadata.full_path = path.string();
		result.status = ScanStatus::SKIPPED;
		result.warnings.emplace_back("UNSUPPORTED_ENTRY", "Only files and directories are scanned.");
		results->push_back(std::move(result));
		return;
	}

	std::filesystem::recursive_directory_iterator it(
		path,
		std::filesystem::directory_options::skip_permission_denied,
		error);
	std::filesystem::recursive_directory_iterator end;
	for (; it != end; it.increment(error)) {
		if (error) {
			Logger::warning("ScannerModule: failed to iterate %s: %s", path.string().c_str(), error.message().c_str());
			error.clear();
			continue;
		}

		const auto& entry_path = it->path();
		const std::string filename = entry_path.filename().string();
		if (!params.include_hidden && !filename.empty() && filename.front() == '.') {
			if (it->is_directory(error)) {
				it.disable_recursion_pending();
			}
			continue;
		}

		if (it->is_regular_file(error)) {
			results->push_back(scan_file(entry_path, params));
		}
	}
}

ScanResult ScannerModule::scan_file(const std::filesystem::path& path, const ScanParameters& params) const {
	ScanResult result;
	result.file_path = path.string();
	result.metadata.full_path = path.string();
	result.metadata.exists = true;
	result.metadata.is_directory = false;
	result.metadata.extension = path.extension().string();

	std::error_code error;
	result.metadata.size = std::filesystem::file_size(path, error);
	if (error) {
		result.status = ScanStatus::ERROR;
		result.error_message = error.message();
		result.errors.emplace_back("FILE_SIZE_FAILED", error.message());
		return result;
	}

	if (result.metadata.size == 0u) {
		result.warnings.emplace_back("EMPTY_FILE", "File has zero size.");
	}

	if (path.string().find(' ') != std::string::npos) {
		result.warnings.emplace_back("PATH_WITH_SPACES", "Path contains spaces and may complicate tooling.");
	}

	const bool source_like = is_source_like(path);
	if (source_like && (params.analyze_dependencies || params.analyze_syntax)) {
		ParseSourceFile(
			path,
			params.analyze_dependencies,
			params.analyze_syntax,
			&result.dependencies,
			&result.warnings);
	}

	if (!result.errors.empty()) {
		result.status = ScanStatus::ERROR;
	} else if (!result.warnings.empty()) {
		result.status = ScanStatus::WARNING;
	} else {
		result.status = ScanStatus::SUCCESS;
	}

	return result;
}

bool ScannerModule::is_source_like(const std::filesystem::path& path) {
	const std::string extension = ToLower(path.extension().string());
	return extension == ".c" || extension == ".cc" || extension == ".cpp" ||
		   extension == ".cxx" || extension == ".h" || extension == ".hh" ||
		   extension == ".hpp" || extension == ".hxx" || extension == ".js" ||
		   extension == ".ts" || extension == ".py" || extension == ".md";
}

} // namespace EngineDoctor
