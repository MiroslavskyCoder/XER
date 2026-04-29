#include "flux/terminal/terminal_output_renderer.h"

#include "flux/terminal/terminal_manager.h"

namespace flux::terminal {

void Write(TerminalInterface& terminal, OutputStream stream, std::string_view text) {
	terminal.Write(stream, text);
}

void Write(OutputStream stream, std::string_view text) {
	TerminalManager::Instance().Write(stream, text);
}

void WriteLine(TerminalInterface& terminal, OutputStream stream, std::string_view text) {
	terminal.Write(stream, text);
	terminal.Write(stream, "\n");
	terminal.Flush(stream);
}

void WriteLine(OutputStream stream, std::string_view text) {
	TerminalManager::Instance().WriteLine(stream, text);
}

std::string Snapshot(OutputStream stream) {
	return TerminalManager::Instance().buffer().Snapshot(stream);
}

}  // namespace flux::terminal
