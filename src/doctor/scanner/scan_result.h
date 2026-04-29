/**
 * @file scan_result.h
 * @brief Scan result model shared by scanner and analysis.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace EngineDoctor {

enum class ScanStatus {
	PENDING,
	SUCCESS,
	WARNING,
	ERROR,
	SKIPPED,
};

struct ScanMessage {
	std::string code;
	std::string message;

	ScanMessage() = default;
	ScanMessage(std::string code_in, std::string message_in)
		: code(std::move(code_in)),
		  message(std::move(message_in)) {}
};

struct FileMetadata {
	std::string full_path;
	std::uintmax_t size = 0;
	bool exists = false;
	bool is_directory = false;
	std::string extension;
};

struct ScanResult {
	std::string file_path;
	FileMetadata metadata;
	ScanStatus status = ScanStatus::PENDING;
	std::string error_message;
	std::vector<ScanMessage> errors;
	std::vector<ScanMessage> warnings;
	std::vector<std::string> dependencies;
};

} // namespace EngineDoctor
