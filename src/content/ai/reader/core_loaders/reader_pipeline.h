#pragma once

#include "reader_base.h"

#include "../schema_validation/metadata_extractor.h"
#include "../schema_validation/schema_validator.h"

#include <string>

namespace Engine::ModelsBuilder::Reader {

struct PipelineQualityResult {
	LoadResult load_result;
	Schema::SchemaValidator::ValidationResult validation;
	Schema::ModelMetadata metadata;
	bool integrity_ok = false;
	bool passed = false;
	std::string summary;
};

class ReaderPipeline {
 public:
	static LoadResult Process(const std::string& filepath);
	static PipelineQualityResult ProcessWithQuality(const std::string& filepath);
};

}  // namespace Engine::ModelsBuilder::Reader

