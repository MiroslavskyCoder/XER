/**
 * @file scanner_module.h
 * @brief Project scanner that materializes scan results into the shared context.
 */

#pragma once

#include "core/engine_doctor_config.h"
#include "core/engine_doctor_context.h"
#include "scanner/scan_parameters.h"
#include "scanner/scan_result.h"

#include <filesystem>
#include <vector>

namespace EngineDoctor {

class ScannerModule {
public:
	explicit ScannerModule(Context& context);

	void initialize(const Config& config);
	void scan(const ScanParameters& params);

private:
	void scan_path(const std::filesystem::path& path, const ScanParameters& params, std::vector<ScanResult>* results) const;
	ScanResult scan_file(const std::filesystem::path& path, const ScanParameters& params) const;
	static bool is_source_like(const std::filesystem::path& path);

	Context& context_;
};

} // namespace EngineDoctor
