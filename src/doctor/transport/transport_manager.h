#pragma once
#include "transport/channel_factory.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace EngineDoctor {

class TransportManager {
public:
    void RegisterChannel(const std::string& name, std::shared_ptr<IChannel> ch);
    bool Send(const std::string& channel, const std::string& data);
    std::string Receive(const std::string& channel);

private:
    std::unordered_map<std::string, std::shared_ptr<IChannel>> channels_;
};

}  // namespace EngineDoctor
