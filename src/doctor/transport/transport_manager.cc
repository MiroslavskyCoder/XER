#include "transport/transport_manager.h"

namespace EngineDoctor {

void TransportManager::RegisterChannel(const std::string& name,
                                       std::shared_ptr<IChannel> ch) {
    channels_[name] = std::move(ch);
}

bool TransportManager::Send(const std::string& channel, const std::string& data) {
    auto it = channels_.find(channel);
    if (it == channels_.end() || !it->second) return false;
    return it->second->Send(data);
}

std::string TransportManager::Receive(const std::string& channel) {
    auto it = channels_.find(channel);
    if (it == channels_.end() || !it->second) return "";
    return it->second->Receive();
}

}  // namespace EngineDoctor
