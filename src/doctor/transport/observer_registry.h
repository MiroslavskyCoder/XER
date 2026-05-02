#pragma once
#include "transport/transport_weak_observer.h"
#include <memory>
#include <vector>

namespace EngineDoctor {

class ObserverRegistry {
public:
    void Register(std::shared_ptr<TransportWeakObserver> obs);
    void Unregister(const TransportWeakObserver* obs);
    void Notify(const std::string& event, const std::string& data);

private:
    std::vector<std::weak_ptr<TransportWeakObserver>> observers_;
};

}  // namespace EngineDoctor
