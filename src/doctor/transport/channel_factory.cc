#include "transport/channel_factory.h"

namespace EngineDoctor {

std::unique_ptr<IChannel> ChannelFactory::Create(ChannelType /*type*/) {
    // Channels not yet implemented.
    return nullptr;
}

}  // namespace EngineDoctor
