#include "flux/terminal/terminal_interface.h"
#include "flux/terminal/terminal_emulator.h"
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
	const std::string esc = std::string("\x1b", 1);
	const std::string c1_dcs = std::string("\x90", 1) + "drop-c1" + std::string("\x9c", 1);

	flux::terminal::SetDefaultTerminal(std::make_unique<SilentTerminal>());
	flux::terminal::ClearSnapshot();

	flux::terminal::Write(OutputStream::kStdout, esc + "[31mred" + esc + "[0m ");
	flux::terminal::Write(OutputStream::kStdout, c1_dcs);
	flux::terminal::Write(OutputStream::kStdout, esc + "Pdrop-dcs" + esc + "\\");
	flux::terminal::Write(OutputStream::kStdout, esc + "_ignored-apc" + esc + "\\");
	flux::terminal::Write(OutputStream::kStdout, esc + "^ignored-pm" + esc + "\\");
	flux::terminal::Write(OutputStream::kStdout, esc + "Xignored-sos" + esc + "\\");
	flux::terminal::WriteLine(OutputStream::kStdout, esc + "[1;32mgreen" + esc + "[0m");
	flux::terminal::WriteLine(OutputStream::kStderr, esc + "]0;warn-title\awarn: " + esc + "[33mamber" + esc + "[0m" + esc + "Pdrop" + esc + "\\");

	const std::string raw_stdout = flux::terminal::Snapshot(OutputStream::kStdout);
	const std::string raw_stderr = flux::terminal::Snapshot(OutputStream::kStderr);
	const std::string plain_stdout = flux::terminal::SnapshotPlainText(OutputStream::kStdout);
	const std::string plain_stderr = flux::terminal::SnapshotPlainText(OutputStream::kStderr);
	const bool direct_emulator_ok = flux::terminal::EmulatePlainText(raw_stdout) == plain_stdout
		&& flux::terminal::EmulatePlainText(raw_stderr) == plain_stderr;

	const bool raw_stdout_ok = raw_stdout.find(esc + "[31m") != std::string::npos
		&& raw_stdout.find(c1_dcs) != std::string::npos
		&& raw_stdout.find(esc + "Pdrop-dcs" + esc + "\\") != std::string::npos
		&& raw_stdout.find(esc + "_ignored-apc" + esc + "\\") != std::string::npos
		&& raw_stdout.find(esc + "^ignored-pm" + esc + "\\") != std::string::npos
		&& raw_stdout.find(esc + "Xignored-sos" + esc + "\\") != std::string::npos;
	const bool raw_stderr_ok = raw_stderr.find(esc + "]0;warn-title\a") != std::string::npos
		&& raw_stderr.find(esc + "[33m") != std::string::npos
		&& raw_stderr.find(esc + "Pdrop" + esc + "\\") != std::string::npos;
	const bool plain_stdout_ok = plain_stdout == "red green\n";
	const bool plain_stderr_ok = plain_stderr == "warn: amber\n";

	flux::terminal::ClearSnapshot(OutputStream::kStdout);
	flux::terminal::Write(OutputStream::kStdout, "AB");
	flux::terminal::Write(OutputStream::kStdout, esc + "[3C");
	flux::terminal::Write(OutputStream::kStdout, "Z");
	flux::terminal::Write(OutputStream::kStdout, esc + "[2D");
	flux::terminal::WriteLine(OutputStream::kStdout, "Y");
	const bool cursor_horizontal_ok = flux::terminal::SnapshotPlainText(OutputStream::kStdout) == "AB  YZ\n";

	flux::terminal::ClearSnapshot(OutputStream::kStdout);
	flux::terminal::Write(OutputStream::kStdout, "12345");
	flux::terminal::Write(OutputStream::kStdout, esc + "[2D");
	flux::terminal::Write(OutputStream::kStdout, esc + "[K");
	flux::terminal::WriteLine(OutputStream::kStdout, "XY");
	const bool erase_line_ok = flux::terminal::SnapshotPlainText(OutputStream::kStdout) == "123XY\n";

	flux::terminal::ClearSnapshot(OutputStream::kStdout);
	flux::terminal::WriteLine(OutputStream::kStdout, "top");
	flux::terminal::WriteLine(OutputStream::kStdout, "middle");
	flux::terminal::Write(OutputStream::kStdout, "\r");
	flux::terminal::Write(OutputStream::kStdout, esc + "[2A");
	flux::terminal::Write(OutputStream::kStdout, "X");
	flux::terminal::Write(OutputStream::kStdout, "\r");
	flux::terminal::Write(OutputStream::kStdout, esc + "[2B");
	flux::terminal::WriteLine(OutputStream::kStdout, "bottom");
	const bool cursor_vertical_ok = flux::terminal::SnapshotPlainText(OutputStream::kStdout) == "Xop\nmiddle\nbottom\n";

	flux::terminal::ClearSnapshot(OutputStream::kStdout);
	flux::terminal::WriteLine(OutputStream::kStdout, "first");
	flux::terminal::Write(OutputStream::kStdout, "second");
	flux::terminal::Write(OutputStream::kStdout, "\r");
	flux::terminal::Write(OutputStream::kStdout, esc + "[J");
	flux::terminal::WriteLine(OutputStream::kStdout, "fresh");
	const bool erase_display_ok = flux::terminal::SnapshotPlainText(OutputStream::kStdout) == "first\nfresh\n";

	flux::terminal::ClearSnapshot(OutputStream::kStdout);
	const bool clear_stdout_ok = flux::terminal::Snapshot(OutputStream::kStdout).empty()
		&& flux::terminal::SnapshotPlainText(OutputStream::kStdout).empty();
	const bool stderr_preserved_ok = flux::terminal::SnapshotPlainText(OutputStream::kStderr) == "warn: amber\n";

	const bool all_ok = raw_stdout_ok && raw_stderr_ok && plain_stdout_ok && plain_stderr_ok
		&& direct_emulator_ok && cursor_horizontal_ok && erase_line_ok
		&& cursor_vertical_ok && erase_display_ok && clear_stdout_ok && stderr_preserved_ok;
	std::cout
		<< "{"
		<< "\"rawStdoutOk\":" << JsonBool(raw_stdout_ok) << ","
		<< "\"rawStderrOk\":" << JsonBool(raw_stderr_ok) << ","
		<< "\"plainStdoutOk\":" << JsonBool(plain_stdout_ok) << ","
		<< "\"plainStderrOk\":" << JsonBool(plain_stderr_ok) << ","
		<< "\"directEmulatorOk\":" << JsonBool(direct_emulator_ok) << ","
		<< "\"cursorHorizontalOk\":" << JsonBool(cursor_horizontal_ok) << ","
		<< "\"eraseLineOk\":" << JsonBool(erase_line_ok) << ","
		<< "\"cursorVerticalOk\":" << JsonBool(cursor_vertical_ok) << ","
		<< "\"eraseDisplayOk\":" << JsonBool(erase_display_ok) << ","
		<< "\"clearStdoutOk\":" << JsonBool(clear_stdout_ok) << ","
		<< "\"stderrPreservedOk\":" << JsonBool(stderr_preserved_ok)
		<< "}" << std::endl;
	return all_ok ? 0 : 1;
}