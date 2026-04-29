#include "analysis/data_provider.h"

namespace EngineDoctor {

DataProvider::DataProvider(Context& context)
	: context_(context) {}

Config DataProvider::resolved_config() const {
	const auto* config = context_.get_data<Config>(kContextConfigDataKey);
	return NormalizeConfig(config == nullptr ? Config{} : *config);
}

const ScanParameters* DataProvider::scan_parameters() const {
	const Config config = resolved_config();
	return context_.get_data<ScanParameters>(config.scan_parameters_key);
}

const std::vector<ScanResult>* DataProvider::scan_results() const {
	const Config config = resolved_config();
	return context_.get_data<std::vector<ScanResult>>(config.scan_results_key);
}

std::vector<ScanResult> DataProvider::scan_results_copy() const {
	const auto* results = scan_results();
	return results == nullptr ? std::vector<ScanResult>{} : *results;
}

bool DataProvider::has_scan_results() const {
	const auto* results = scan_results();
	return results != nullptr && !results->empty();
}

} // namespace EngineDoctor
