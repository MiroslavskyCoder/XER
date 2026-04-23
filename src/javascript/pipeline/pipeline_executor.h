#pragma once

#include "javascript/events/event_router.h"

#include <cstdint>
#include <string>
#include <vector>

namespace engine::javascript::pipeline {

class PipelineExecutor {
public:
    explicit PipelineExecutor(engine::javascript::events::EventRouter* router);

    std::size_t PublishRaw(const std::string& topic, const std::string& text);
    std::size_t PublishCompressed(const std::string& topic, const std::vector<std::uint8_t>& compressed);

private:
    engine::javascript::events::EventRouter* router_;
};

}  // namespace engine::javascript::pipeline
