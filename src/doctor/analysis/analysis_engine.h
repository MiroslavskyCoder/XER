/**
 * @file analysis_engine.h
 * @brief High-level analysis module for EngineDoctor.
 */

#pragma once

#include "analysis/analysis_pipeline.h"
#include "analysis/analysis_result_handler.h"
#include "core/engine_doctor_config.h"
#include "core/engine_doctor_context.h"

namespace EngineDoctor {

class AnalysisEngine {
public:
	explicit AnalysisEngine(Context& context);

	void initialize(const Config& config);
	void analyze();

private:
	Context& context_;
	AnalysisPipeline pipeline_;
	AnalysisResultHandler result_handler_;
};

} // namespace EngineDoctor
