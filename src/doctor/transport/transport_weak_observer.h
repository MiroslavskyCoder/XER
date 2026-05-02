#pragma once
#include <string>

namespace EngineDoctor {

class TransportWeakObserver {
public:
    virtual ~TransportWeakObserver() = default;
    virtual void OnEvent(const std::string& event, const std::string& data) = 0;
};

}  // namespace EngineDoctor
