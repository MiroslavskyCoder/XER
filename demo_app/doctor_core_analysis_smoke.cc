#include "analysis/analysis_result_handler.h"
#include "core/engine_doctor_config.h"
#include "core/engine_doctor_core.h"
#include "scanner/scan_parameters.h"

#include "flux/terminal/terminal_interface.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace {

class SilentTerminal final : public flux::terminal::TerminalInterface {
public:
	void Write(flux::terminal::OutputStream /*stream*/, std::string_view /*text*/) override {}
	void Flush(flux::terminal::OutputStream /*stream*/) override {}
};

const char* JsonBool(bool value) {
	return value ? "true" : "false";
}

} // namespace

int main() {
	flux::terminal::SetDefaultTerminal(std::make_unique<SilentTerminal>());

	const std::filesystem::path smoke_root = std::filesystem::temp_directory_path() / "xer_doctor_core_smoke";
	std::error_code error;
	std::filesystem::remove_all(smoke_root, error);
	std::filesystem::create_directories(smoke_root, error);

	const std::filesystem::path source_file = smoke_root / "sample.cc";
	const std::filesystem::path empty_file = smoke_root / "empty.ts";
	{
		std::ofstream output(source_file);
		output << "#include <vector>\n"
		       << "int main() { std::vector<int> values; return values.empty() ? 0 : 1; }\n";
	}
	{
		std::ofstream output(empty_file);
	}

	EngineDoctor::Config config;
	config.log_level = 0;
	config.analysis_strategy = "DependencyCheckStrategy";
	config.publish_events = true;
	config.report_output_path = (smoke_root / "report.txt").string();

	EngineDoctor::EngineDoctorCore core(config);
	EngineDoctor::ScanParameters params;
	params.paths_to_scan.push_back(smoke_root.string());
	params.analyze_dependencies = true;
	params.analyze_syntax = true;
	params.include_hidden = true;
	core.run(params);

	const auto* summary = core.context().get_data<EngineDoctor::AnalysisSummary>(core.config().analysis_summary_key);
	const auto* summary_text = core.context().get_data<std::string>(core.config().analysis_summary_text_key);
	const auto* strategy_name = core.context().get_data<std::string>("analysis_strategy_name");
	const std::vector<EngineDoctor::Event> events = core.context().event_bus().history();

	bool saw_run_start = false;
	bool saw_scan_complete = false;
	bool saw_analysis_complete = false;
	for (const auto& event : events) {
		saw_run_start = saw_run_start || event.topic == "doctor.core.run.started";
		saw_scan_complete = saw_scan_complete || event.topic == "doctor.scan.completed";
		saw_analysis_complete = saw_analysis_complete || event.topic == "doctor.analysis.completed";
	}

	const bool summary_ok = summary != nullptr
		&& summary->metrics.total_files == 2
		&& summary->metrics.empty_files == 1
		&& summary->metrics.total_dependencies >= 1
		&& !summary->task_summaries.empty();
	const bool summary_text_ok = summary_text != nullptr
		&& summary_text->find("EngineDoctor Analysis Summary") != std::string::npos
		&& summary_text->find("analysis_strategy=DependencyCheckStrategy") != std::string::npos;
	const bool strategy_ok = strategy_name != nullptr && *strategy_name == "DependencyCheckStrategy";
	const bool event_ok = !events.empty() && saw_run_start && saw_scan_complete && saw_analysis_complete;

	const bool all_ok = summary_ok && summary_text_ok && strategy_ok && event_ok;
	if (!all_ok) {
		std::cerr
			<< "{"
			<< "\"summaryOk\":" << JsonBool(summary_ok) << ","
			<< "\"summaryTextOk\":" << JsonBool(summary_text_ok) << ","
			<< "\"strategyOk\":" << JsonBool(strategy_ok) << ","
			<< "\"eventOk\":" << JsonBool(event_ok)
			<< "}" << std::endl;
	}

	std::filesystem::remove_all(smoke_root, error);
	return all_ok ? 0 : 1;
}