#pragma once
#include <string>

namespace EngineDoctor {

class SecureCommunicationManager {
public:
    bool Connect(const std::string& host, int port);
    bool Send(const std::string& data);
    std::string Receive();
    void Disconnect();

private:
    bool connected_ = false;
};

}  // namespace EngineDoctor
