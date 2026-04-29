#include "flux/terminal/terminal_manager.h"

#include "flux/terminal/terminal_interface.h"

namespace flux::terminal {

TerminalManager& TerminalManager::Instance() {
	static TerminalManager manager;
	return manager;
}

void TerminalManager::Write(OutputStream stream, std::string_view text) {
	std::lock_guard<std::mutex> lock(mutex_);
	buffer_.Append(stream, text);
	GetDefaultTerminal().Write(stream, text);
}

void TerminalManager::WriteLine(OutputStream stream, std::string_view text) {
	std::lock_guard<std::mutex> lock(mutex_);
	buffer_.AppendLine(stream, text);
	TerminalInterface& terminal = GetDefaultTerminal();
	terminal.Write(stream, text);
	terminal.Write(stream, "\n");
	terminal.Flush(stream);
}

void TerminalManager::Flush(OutputStream stream) {
	std::lock_guard<std::mutex> lock(mutex_);
	GetDefaultTerminal().Flush(stream);
}

void TerminalManager::ResetBuffer() {
	std::lock_guard<std::mutex> lock(mutex_);
	buffer_.Clear();
}

const TerminalBuffer& TerminalManager::buffer() const {
	return buffer_;
}

}  // namespace flux::terminal
