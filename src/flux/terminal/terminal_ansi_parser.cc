#include "flux/terminal/terminal_ansi_parser.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace flux::terminal {
namespace {

std::size_t SkipCsiSequence(std::string_view text, std::size_t index) {
	std::size_t cursor = index + 2;
	while (cursor < text.size()) {
		const unsigned char ch = static_cast<unsigned char>(text[cursor]);
		if (ch >= 0x40u && ch <= 0x7Eu) {
			return cursor + 1;
		}
		++cursor;
	}
	return text.size();
}

std::size_t SkipOscSequence(std::string_view text, std::size_t index) {
	std::size_t cursor = index + 2;
	while (cursor < text.size()) {
		if (text[cursor] == '\a') {
			return cursor + 1;
		}
		if (text[cursor] == '\x1b' && cursor + 1 < text.size() && text[cursor + 1] == '\\') {
			return cursor + 2;
		}
		++cursor;
	}
	return text.size();
}

}  // namespace

std::string StripAnsi(std::string_view text) {
	std::string out;
	out.reserve(text.size());
	std::size_t index = 0;
	while (index < text.size()) {
		if (text[index] != '\x1b') {
			out.push_back(text[index]);
			++index;
			continue;
		}
		if (index + 1 >= text.size()) {
			break;
		}
		if (text[index + 1] == '[') {
			index = SkipCsiSequence(text, index);
			continue;
		}
		if (text[index + 1] == ']') {
			index = SkipOscSequence(text, index);
			continue;
		}
		index += 2;
	}
	return out;
}

}  // namespace flux::terminal
