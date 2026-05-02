#include "transport/observer_registry.h"
#include <algorithm>

namespace EngineDoctor {

void ObserverRegistry::Register(std::shared_ptr<TransportWeakObserver> obs) {
    observers_.emplace_back(obs);
}

void ObserverRegistry::Unregister(const TransportWeakObserver* obs) {
    observers_.erase(
        std::remove_if(observers_.begin(), observers_.end(),
            [obs](const std::weak_ptr<TransportWeakObserver>& wp) {
                auto sp = wp.lock();
                return !sp || sp.get() == obs;
            }),
        observers_.end());
}

void ObserverRegistry::Notify(const std::string& event, const std::string& data) {
    std::vector<std::weak_ptr<TransportWeakObserver>> alive;
    for (auto& wp : observers_) {
        if (auto sp = wp.lock()) {
            sp->OnEvent(event, data);
            alive.emplace_back(wp);
        }
    }
    observers_ = std::move(alive);
}

}  // namespace EngineDoctor
