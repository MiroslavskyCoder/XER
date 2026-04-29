#pragma once

#include <mutex>
#include <string_view>

#include "flux/terminal/terminal_buffer.h"

namespace flux::terminal {

class TerminalManager {
public:
	static TerminalManager& Instance();

	void Write(OutputStream stream, std::string_view text);
	void WriteLine(OutputStream stream, std::string_view text);
	void Flush(OutputStream stream);
	void ResetBuffer();

	const TerminalBuffer& buffer() const;

private:
	TerminalManager() = default;

	mutable std::mutex mutex_;
	TerminalBuffer buffer_;
};

}  // namespace flux::terminal
