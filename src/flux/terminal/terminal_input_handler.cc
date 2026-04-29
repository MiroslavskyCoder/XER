#include "flux/terminal/terminal_input_handler.h"

#include <string>
#include <string_view>
#include <vector>

namespace flux::terminal {
namespace {

bool IsPrintableByte(unsigned char byte) {
	return byte >= 0x20u && byte != 0x7Fu;
}

std::size_t EscapeSequenceLength(std::string_view buffer, std::size_t index) {
	if (index >= buffer.size() || buffer[index] != '\x1b') {
		return 1;
	}
	if (index + 2 < buffer.size() && buffer[index + 1] == '[') {
		switch (buffer[index + 2]) {
		case 'A':
		case 'B':
		case 'C':
		case 'D':
			return 3;
		default:
			break;
		}
	}
	return 1;
}

}  // namespace

InputEvent ParseInputSequence(std::string_view sequence) {
	InputEvent event;
	event.raw.assign(sequence.data(), sequence.size());
	if (sequence.empty()) {
		return event;
	}
	if (sequence == "\n" || sequence == "\r") {
		event.kind = InputEventKind::kSubmit;
		return event;
	}
	if (sequence == "\x03") {
		event.kind = InputEventKind::kInterrupt;
		return event;
	}
	if (sequence == "\b" || sequence == "\x7f") {
		event.kind = InputEventKind::kBackspace;
		return event;
	}
	if (sequence == "\x1b") {
		event.kind = InputEventKind::kEscape;
		return event;
	}
	if (sequence == "\x1b[A") {
		event.kind = InputEventKind::kArrowUp;
		return event;
	}
	if (sequence == "\x1b[B") {
		event.kind = InputEventKind::kArrowDown;
		return event;
	}
	if (sequence == "\x1b[C") {
		event.kind = InputEventKind::kArrowRight;
		return event;
	}
	if (sequence == "\x1b[D") {
		event.kind = InputEventKind::kArrowLeft;
		return event;
	}
	event.kind = InputEventKind::kText;
	event.text.assign(sequence.data(), sequence.size());
	return event;
}

std::vector<InputEvent> ParseInputBuffer(std::string_view buffer) {
	std::vector<InputEvent> events;
	std::string pending_text;
	auto flush_text = [&]() {
		if (pending_text.empty()) {
			return;
		}
		InputEvent event;
		event.kind = InputEventKind::kText;
		event.text = pending_text;
		event.raw = pending_text;
		events.push_back(std::move(event));
		pending_text.clear();
	};

	for (std::size_t index = 0; index < buffer.size();) {
		const unsigned char byte = static_cast<unsigned char>(buffer[index]);
		if (buffer[index] == '\x1b') {
			flush_text();
			const std::size_t length = EscapeSequenceLength(buffer, index);
			events.push_back(ParseInputSequence(buffer.substr(index, length)));
			index += length;
			continue;
		}
		if (buffer[index] == '\n' || buffer[index] == '\r' || buffer[index] == '\b' || buffer[index] == '\x7f' || buffer[index] == '\x03') {
			flush_text();
			events.push_back(ParseInputSequence(buffer.substr(index, 1)));
			++index;
			continue;
		}
		if (IsPrintableByte(byte) || buffer[index] == '\t') {
			pending_text.push_back(buffer[index]);
		}
		++index;
	}
	flush_text();
	return events;
}

bool IsCommandSubmission(const InputEvent& event) {
	return event.kind == InputEventKind::kSubmit;
}

}  // namespace flux::terminal
