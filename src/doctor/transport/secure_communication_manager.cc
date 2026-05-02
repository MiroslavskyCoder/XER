#include "transport/secure_communication_manager.h"

namespace EngineDoctor {

bool SecureCommunicationManager::Connect(const std::string& /*host*/,
                                         int /*port*/) {
    return false;
}

bool SecureCommunicationManager::Send(const std::string& /*data*/) {
    return false;
}

std::string SecureCommunicationManager::Receive() {
    return "";
}

void SecureCommunicationManager::Disconnect() {
    connected_ = false;
}

}  // namespace EngineDoctor
