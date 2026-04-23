#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace engine::javascript::common {

struct TaskPayload {
    std::string topic;
    std::string text;
    std::vector<std::uint8_t> binary;
    std::uint64_t sequence = 0;
};

}  // namespace engine::javascript::common
