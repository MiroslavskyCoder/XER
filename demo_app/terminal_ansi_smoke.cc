#include "flux/terminal/terminal_interface.h"
#include "flux/terminal/terminal_output_renderer.h"

#include <iostream>
#include <memory>
#include <string>

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

	flux::terminal::SetDefaultTerminal(std::make_unique<SilentTerminal>());
	flux::terminal::ClearSnapshot();

	flux::terminal::Write(OutputStream::kStdout, "\x1b[31mred\x1b[0m ");
	flux::terminal::WriteLine(OutputStream::kStdout, "\x1b[1;32mgreen\x1b[0m");
	flux::terminal::WriteLine(OutputStream::kStderr, "\x1b]0;warn-title\awarn: \x1b[33mamber\x1b[0m");

	const std::string raw_stdout = flux::terminal::Snapshot(OutputStream::kStdout);
	const std::string raw_stderr = flux::terminal::Snapshot(OutputStream::kStderr);
	const std::string plain_stdout = flux::terminal::SnapshotPlainText(OutputStream::kStdout);
	const std::string plain_stderr = flux::terminal::SnapshotPlainText(OutputStream::kStderr);

	const bool raw_stdout_ok = raw_stdout.find("\x1b[31m") != std::string::npos
		&& raw_stdout.find("\x1b[1;32m") != std::string::npos;
	const bool raw_stderr_ok = raw_stderr.find("\x1b]0;warn-title\a") != std::string::npos
		&& raw_stderr.find("\x1b[33m") != std::string::npos;
	const bool plain_stdout_ok = plain_stdout == "red green\n";
	const bool plain_stderr_ok = plain_stderr == "warn: amber\n";

	flux::terminal::ClearSnapshot(OutputStream::kStdout);
	const bool clear_stdout_ok = flux::terminal::Snapshot(OutputStream::kStdout).empty()
		&& flux::terminal::SnapshotPlainText(OutputStream::kStdout).empty();
	const bool stderr_preserved_ok = flux::terminal::SnapshotPlainText(OutputStream::kStderr) == "warn: amber\n";

	const bool all_ok = raw_stdout_ok && raw_stderr_ok && plain_stdout_ok && plain_stderr_ok
		&& clear_stdout_ok && stderr_preserved_ok;
	std::cout
		<< "{"
		<< "\"rawStdoutOk\":" << JsonBool(raw_stdout_ok) << ","
		<< "\"rawStderrOk\":" << JsonBool(raw_stderr_ok) << ","
		<< "\"plainStdoutOk\":" << JsonBool(plain_stdout_ok) << ","
		<< "\"plainStderrOk\":" << JsonBool(plain_stderr_ok) << ","
		<< "\"clearStdoutOk\":" << JsonBool(clear_stdout_ok) << ","
		<< "\"stderrPreservedOk\":" << JsonBool(stderr_preserved_ok)
		<< "}" << std::endl;
	return all_ok ? 0 : 1;
}