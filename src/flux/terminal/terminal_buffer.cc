#include "flux/terminal/terminal_buffer.h"

namespace flux::terminal {
namespace {

std::vector<std::string> SplitLines(const std::string& text) {
	std::vector<std::string> lines;
	std::size_t start = 0;
	while (start < text.size()) {
		const std::size_t newline = text.find('\n', start);
		if (newline == std::string::npos) {
			lines.push_back(text.substr(start));
			return lines;
		}
		lines.push_back(text.substr(start, newline - start));
		start = newline + 1;
	}
	if (!text.empty() && text.back() == '\n') {
		lines.emplace_back();
	}
	return lines;
}

}  // namespace

std::string& TerminalBuffer::MutableStorage(OutputStream stream) {
	return stream == OutputStream::kStdout ? stdout_buffer_ : stderr_buffer_;
}

const std::string& TerminalBuffer::Storage(OutputStream stream) const {
	return stream == OutputStream::kStdout ? stdout_buffer_ : stderr_buffer_;
}

void TerminalBuffer::Append(OutputStream stream, std::string_view text) {
	MutableStorage(stream).append(text.data(), text.size());
}

void TerminalBuffer::AppendLine(OutputStream stream, std::string_view text) {
	Append(stream, text);
	MutableStorage(stream).push_back('\n');
}

void TerminalBuffer::Clear() {
	stdout_buffer_.clear();
	stderr_buffer_.clear();
}

void TerminalBuffer::Clear(OutputStream stream) {
	MutableStorage(stream).clear();
}

std::string TerminalBuffer::Snapshot(OutputStream stream) const {
	return Storage(stream);
}

std::vector<std::string> TerminalBuffer::Lines(OutputStream stream) const {
	return SplitLines(Storage(stream));
}

}  // namespace flux::terminal
