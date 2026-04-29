#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace flux::terminal {

enum class InputEventKind {
	kUnknown,
	kText,
	kSubmit,
	kInterrupt,
	kBackspace,
	kArrowUp,
	kArrowDown,
	kArrowLeft,
	kArrowRight,
	kEscape,
};

struct InputEvent {
	InputEventKind kind = InputEventKind::kUnknown;
	std::string text;
	std::string raw;
};

InputEvent ParseInputSequence(std::string_view sequence);
std::vector<InputEvent> ParseInputBuffer(std::string_view buffer);
bool IsCommandSubmission(const InputEvent& event);

}  // namespace flux::terminal
