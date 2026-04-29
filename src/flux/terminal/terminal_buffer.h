#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "flux/terminal/terminal_interface.h"

namespace flux::terminal {

class TerminalBuffer {
public:
	void Append(OutputStream stream, std::string_view text);
	void AppendLine(OutputStream stream, std::string_view text);
	void Clear();
	void Clear(OutputStream stream);

	std::string Snapshot(OutputStream stream) const;
	std::vector<std::string> Lines(OutputStream stream) const;

private:
	std::string& MutableStorage(OutputStream stream);
	const std::string& Storage(OutputStream stream) const;

	std::string stdout_buffer_;
	std::string stderr_buffer_;
};

}  // namespace flux::terminal
