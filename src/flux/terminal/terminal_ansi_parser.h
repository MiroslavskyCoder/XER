#pragma once

#include <string>
#include <string_view>

namespace flux::terminal {

std::string StripAnsi(std::string_view text);

}  // namespace flux::terminal
