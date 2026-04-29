#pragma once

#include "analysis/analysis_pipeline.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace EngineDoctor {

class AnalysisStrategy {
public:
	virtual ~AnalysisStrategy() = default;

	virtual std::string name() const = 0;
	virtual AnalysisSummary execute(
		const std::vector<ScanResult>& scan_results,
		AnalysisPipeline& pipeline) const = 0;
};

std::unique_ptr<AnalysisStrategy> CreateAnalysisStrategy(std::string_view strategy_name);

} // namespace EngineDoctor
