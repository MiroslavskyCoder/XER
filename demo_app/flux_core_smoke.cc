#include "flux/core/flux_core.h"
#include "flux/terminal/terminal_input_handler.h"
#include "flux/terminal/terminal_interface.h"
#include "flux/terminal/terminal_output_renderer.h"

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

}  // namespace

int main() {
	using flux::terminal::InputEventKind;
	using flux::terminal::OutputStream;

	flux::terminal::SetDefaultTerminal(std::make_unique<SilentTerminal>());
	flux::terminal::ClearSnapshot();

	flux::core::FluxConfig config;
	config.application_name = "flux-core-smoke";
	config.log_level = flux::core::LogLevel::kDebug;
	config.colors = false;
	config.timestamps = false;
	config.terminal_size.columns = 32;
	config.terminal_size.rows = 8;

	flux::core::FluxCore core(config);
	std::string error;
	const bool register_ok = core.RegisterModule(
		flux::core::FluxModule{
			"state",
			[](flux::core::FluxContext& context, std::string* /*error_out*/) {
				context.SetValue("module.state", "started");
				return true;
			},
			[](flux::core::FluxContext& context) {
				context.SetValue("module.state", "stopped");
			}},
		&error);
	const bool init_ok = register_ok && core.Initialize(&error);

	const std::vector<flux::terminal::InputEvent> events = flux::terminal::ParseInputBuffer("run\n\x1b[A\x03");
	const bool input_ok = events.size() == 4
		&& events[0].kind == InputEventKind::kText && events[0].text == "run"
		&& events[1].kind == InputEventKind::kSubmit
		&& events[2].kind == InputEventKind::kArrowUp
		&& events[3].kind == InputEventKind::kInterrupt;

	const bool module_started_ok = core.context().GetValue("module.state") == "started";
	const bool rule_ok = core.context().window().MakeRule('=').size() == core.context().window().size().columns;
	const bool centered_ok = core.context().window().Centered("flux").size() == core.context().window().size().columns;
	const std::string stdout_snapshot = flux::terminal::SnapshotPlainText(OutputStream::kStdout);
	const bool log_ok = stdout_snapshot.find("FluxCore initialized") != std::string::npos;

	core.Shutdown();
	const bool module_stopped_ok = core.context().GetValue("module.state") == "stopped";

	const bool all_ok = register_ok && init_ok && input_ok && module_started_ok
		&& rule_ok && centered_ok && log_ok && module_stopped_ok;

	std::cout
		<< "{"
		<< "\"registerOk\":" << JsonBool(register_ok) << ","
		<< "\"initOk\":" << JsonBool(init_ok) << ","
		<< "\"inputOk\":" << JsonBool(input_ok) << ","
		<< "\"moduleStartedOk\":" << JsonBool(module_started_ok) << ","
		<< "\"ruleOk\":" << JsonBool(rule_ok) << ","
		<< "\"centeredOk\":" << JsonBool(centered_ok) << ","
		<< "\"logOk\":" << JsonBool(log_ok) << ","
		<< "\"moduleStoppedOk\":" << JsonBool(module_stopped_ok)
		<< "}" << std::endl;
	return all_ok ? 0 : 1;
}