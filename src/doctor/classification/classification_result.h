#pragma once

#include <string>
#include <vector>

namespace EngineDoctor {

enum class ClassificationStatus {
	UNKNOWN,
	MATCHED,
	NOT_MATCHED,
	ERROR
};

struct ClassificationResult {
	std::string file_path;
	ClassificationStatus status = ClassificationStatus::UNKNOWN;
	std::vector<std::string> matched_rules;
	std::string error_message;
};

} // namespace EngineDoctor
