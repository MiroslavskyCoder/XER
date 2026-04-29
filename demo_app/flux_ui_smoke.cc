#include "flux/history/action_history.h"
#include "flux/input/input_manager.h"
#include "flux/rendering/render_context.h"
#include "flux/rendering/render_pipeline.h"
#include "flux/rendering/renderer.h"
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
	using flux::input::InputDispatchKind;
	using flux::terminal::OutputStream;

	flux::terminal::SetDefaultTerminal(std::make_unique<SilentTerminal>());
	flux::terminal::ClearSnapshot();

	flux::history::ActionHistory actions(4);
	actions.Push("input", "dispatch command");
	actions.Push("render", "present frame");
	const flux::history::ActionRecord* latest_action = actions.Latest();
	const bool action_ok = actions.size() == 2 && latest_action != nullptr
		&& latest_action->sequence == 2 && latest_action->category == "render";

	flux::input::InputManager input;
	std::string binding_error;
	const bool binding_register_ok = input.bindings().Register("\x1b", "cancel", "Cancel current operation", &binding_error);
	const std::vector<flux::input::InputDispatch> command_dispatches =
		input.Feed("deploy \"demo app\" --target=smoke --watch\n");
	const bool command_ok = command_dispatches.size() == 1
		&& command_dispatches[0].kind == InputDispatchKind::kCommand
		&& command_dispatches[0].parsed_command.valid
		&& command_dispatches[0].parsed_command.name == "deploy"
		&& command_dispatches[0].parsed_command.args.size() == 1
		&& command_dispatches[0].parsed_command.args[0] == "demo app"
		&& command_dispatches[0].parsed_command.OptionValue("target") == "smoke"
		&& command_dispatches[0].parsed_command.OptionValue("watch") == "true"
		&& input.history().size() == 1;

	const std::vector<flux::input::InputDispatch> history_dispatches = input.Feed("\x1b[A");
	const bool history_ok = history_dispatches.size() == 1
		&& history_dispatches[0].kind == InputDispatchKind::kHistoryRecall
		&& history_dispatches[0].buffer == "deploy \"demo app\" --target=smoke --watch";

	const std::vector<flux::input::InputDispatch> binding_dispatches = input.Feed("\x1b");
	const bool binding_ok = binding_register_ok && binding_dispatches.size() == 1
		&& binding_dispatches[0].kind == InputDispatchKind::kBinding
		&& binding_dispatches[0].binding_command == "cancel";

	flux::rendering::RenderContext context({16, 4, false});
	flux::terminal::TerminalStyle title_style;
	title_style.bold = true;
	title_style.foreground = flux::terminal::TerminalColor::kBrightCyan;
	flux::terminal::TerminalStyle body_style;
	body_style.foreground = flux::terminal::TerminalColor::kBrightGreen;
	context.Clear();
	context.SetTitle("flux-ui");
	context.frame().WriteText(0, 0, "FluxUI", title_style);
	context.frame().WriteText(0, 1, "ready", body_style);

	flux::rendering::Renderer renderer;
	const std::string plain = renderer.RenderPlainText(context);
	const bool plain_ok = plain.find("FluxUI\n") != std::string::npos
		&& plain.find("ready\n") != std::string::npos;

	flux::terminal::ClearSnapshot(OutputStream::kStdout);
	flux::rendering::RenderPipeline pipeline;
	const bool present_first_ok = pipeline.Present(context, OutputStream::kStdout);
	const bool present_second_skipped = !pipeline.Present(context, OutputStream::kStdout);
	context.frame().WriteText(5, 1, " now", body_style);
	const bool present_third_ok = pipeline.Present(context, OutputStream::kStdout);
	const std::string snapshot_plain = flux::terminal::SnapshotPlainText(OutputStream::kStdout);
	const bool render_ok = present_first_ok && present_second_skipped && present_third_ok
		&& pipeline.frame_count() == 2
		&& snapshot_plain.find("FluxUI") != std::string::npos
		&& snapshot_plain.find("ready now") != std::string::npos
		&& pipeline.last_plain_frame().find("ready now") != std::string::npos;

	const bool all_ok = action_ok && command_ok && history_ok && binding_ok && plain_ok && render_ok;
	std::cout
		<< "{"
		<< "\"actionOk\":" << JsonBool(action_ok) << ","
		<< "\"commandOk\":" << JsonBool(command_ok) << ","
		<< "\"historyOk\":" << JsonBool(history_ok) << ","
		<< "\"bindingOk\":" << JsonBool(binding_ok) << ","
		<< "\"plainOk\":" << JsonBool(plain_ok) << ","
		<< "\"renderOk\":" << JsonBool(render_ok)
		<< "}" << std::endl;
	return all_ok ? 0 : 1;
}