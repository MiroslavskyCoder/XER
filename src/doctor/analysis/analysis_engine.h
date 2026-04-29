/**
 * @file analysis_engine.h
 * @brief High-level analysis module for EngineDoctor.
 */

#pragma once

#include "analysis/analysis_strategy.h"
#include "analysis/data_provider.h"
#include "analysis/analysis_pipeline.h"
#include "analysis/analysis_result_handler.h"
#include "core/engine_doctor_config.h"
#include "core/engine_doctor_context.h"

#include <memory>

namespace EngineDoctor {

class AnalysisEngine {
public:
	explicit AnalysisEngine(Context& context);

	void initialize(const Config& config);
	void analyze();

private:
	Context& context_;
	DataProvider data_provider_;
	AnalysisPipeline pipeline_;
	AnalysisResultHandler result_handler_;
	std::unique_ptr<AnalysisStrategy> strategy_;
	Config config_;
};

} // namespace EngineDoctor
