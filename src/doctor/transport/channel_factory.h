#pragma once
#include <memory>
#include <string>

namespace EngineDoctor {

enum class ChannelType { Pipe, Socket, SharedMemory };

class IChannel {
public:
    virtual ~IChannel() = default;
    virtual bool Send(const std::string& data) = 0;
    virtual std::string Receive() = 0;
};

class ChannelFactory {
public:
    static std::unique_ptr<IChannel> Create(ChannelType type);
};

}  // namespace EngineDoctor
