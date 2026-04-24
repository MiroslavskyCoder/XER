#include "plugin_parameter_bridge.h"

namespace Engine::Audio::Plugin {

void PluginParameterBridge::RegisterParameter(uint32_t id, const std::string& name, float default_value) {
    (void)name;
    IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    values_[id] = default_value;
}

bool PluginParameterBridge::SetValue(uint32_t id, float value) {
    IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    const auto it = values_.find(id);
    if (it == values_.end()) {
        return false;
    }
    it->second = value;
    return true;
}

bool PluginParameterBridge::GetValue(uint32_t id, float& value) const {
    IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    const auto it = values_.find(id);
    if (it == values_.end()) {
        return false;
    }
    value = it->second;
    return true;
}

}  // namespace Engine::Audio::Plugin
