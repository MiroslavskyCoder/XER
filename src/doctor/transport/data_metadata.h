#pragma once
#include <cstdint>
#include <string>

namespace EngineDoctor {

struct DataMetadata {
    uint64_t id = 0;
    uint64_t timestamp_ms = 0;
    std::string type;
    size_t size = 0;
};

}  // namespace EngineDoctor
