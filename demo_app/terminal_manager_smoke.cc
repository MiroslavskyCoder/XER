#include "flux/terminal/terminal_interface.h"
#include "flux/terminal/terminal_manager.h"

#include <iostream>
#include <memory>

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
	using flux::terminal::OutputStream;
	using flux::terminal::TerminalManager;

	flux::terminal::SetDefaultTerminal(std::make_unique<SilentTerminal>());
	TerminalManager& manager = TerminalManager::Instance();
	manager.ResetBuffer();

	manager.Write(OutputStream::kStdout, "alpha");
	manager.WriteLine(OutputStream::kStdout, "-line");
	manager.WriteLine(OutputStream::kStderr, "err-line");

	const bool stdout_ok = manager.Snapshot(OutputStream::kStdout) == "alpha-line\n";
	const bool stderr_ok = manager.Snapshot(OutputStream::kStderr) == "err-line\n";
	const bool plain_stdout_ok = manager.SnapshotPlainText(OutputStream::kStdout) == "alpha-line\n";
	const bool plain_stderr_ok = manager.SnapshotPlainText(OutputStream::kStderr) == "err-line\n";

	manager.ResetBuffer(OutputStream::kStdout);
	const bool clear_stdout_ok = manager.Snapshot(OutputStream::kStdout).empty();
	const bool clear_plain_stdout_ok = manager.SnapshotPlainText(OutputStream::kStdout).empty();
	const bool stderr_preserved_ok = manager.Snapshot(OutputStream::kStderr) == "err-line\n";
	const bool plain_stderr_preserved_ok = manager.SnapshotPlainText(OutputStream::kStderr) == "err-line\n";

	manager.ResetBuffer();
	const bool clear_all_ok = manager.Snapshot(OutputStream::kStdout).empty()
		&& manager.Snapshot(OutputStream::kStderr).empty()
		&& manager.SnapshotPlainText(OutputStream::kStdout).empty()
		&& manager.SnapshotPlainText(OutputStream::kStderr).empty();

	const bool all_ok = stdout_ok && stderr_ok && plain_stdout_ok && plain_stderr_ok
		&& clear_stdout_ok && clear_plain_stdout_ok && stderr_preserved_ok
		&& plain_stderr_preserved_ok && clear_all_ok;
	std::cout
		<< "{"
		<< "\"stdoutOk\":" << JsonBool(stdout_ok) << ","
		<< "\"stderrOk\":" << JsonBool(stderr_ok) << ","
		<< "\"plainStdoutOk\":" << JsonBool(plain_stdout_ok) << ","
		<< "\"plainStderrOk\":" << JsonBool(plain_stderr_ok) << ","
		<< "\"clearStdoutOk\":" << JsonBool(clear_stdout_ok) << ","
		<< "\"clearPlainStdoutOk\":" << JsonBool(clear_plain_stdout_ok) << ","
		<< "\"stderrPreservedOk\":" << JsonBool(stderr_preserved_ok) << ","
		<< "\"plainStderrPreservedOk\":" << JsonBool(plain_stderr_preserved_ok) << ","
		<< "\"clearAllOk\":" << JsonBool(clear_all_ok)
		<< "}" << std::endl;
	return all_ok ? 0 : 1;
}