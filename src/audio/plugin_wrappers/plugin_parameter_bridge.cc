#include "plugin_parameter_bridge.h"

#include <algorithm>

namespace Engine::Audio::Plugin {

void PluginParameterBridge::Clear() {
    AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    values_.clear();
}

void PluginParameterBridge::RegisterParameter(
    uint32_t id,
    const std::string& name,
    float default_value,
    float min_value,
    float max_value) {
    AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    if (min_value > max_value) {
        std::swap(min_value, max_value);
    }
    const float clamped_default = std::clamp(default_value, min_value, max_value);
    values_[id] = PluginParameterInfo{id, name, min_value, max_value, clamped_default, clamped_default};
}

bool PluginParameterBridge::SetValue(uint32_t id, float value) {
    AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    const auto it = values_.find(id);
    if (it == values_.end()) {
        return false;
    }
    it->second.current_value = std::clamp(value, it->second.min_value, it->second.max_value);
    return true;
}

bool PluginParameterBridge::SetNormalizedValue(uint32_t id, float normalized_value) {
    AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    const auto it = values_.find(id);
    if (it == values_.end()) {
        return false;
    }
    const float clamped = std::clamp(normalized_value, 0.0f, 1.0f);
    it->second.current_value = it->second.min_value + (it->second.max_value - it->second.min_value) * clamped;
    return true;
}

bool PluginParameterBridge::GetValue(uint32_t id, float& value) const {
    AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    const auto it = values_.find(id);
    if (it == values_.end()) {
        return false;
    }
    value = it->second.current_value;
    return true;
}

bool PluginParameterBridge::GetInfo(uint32_t id, PluginParameterInfo* info) const {
    if (info == nullptr) {
        return false;
    }
    AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    const auto it = values_.find(id);
    if (it == values_.end()) {
        return false;
    }
    *info = it->second;
    return true;
}

std::vector<PluginParameterInfo> PluginParameterBridge::Snapshot() const {
    AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    std::vector<PluginParameterInfo> snapshot;
    snapshot.reserve(values_.size());
    for (const auto& entry : values_) {
        snapshot.push_back(entry.second);
    }
    std::sort(snapshot.begin(), snapshot.end(), [](const PluginParameterInfo& lhs, const PluginParameterInfo& rhs) {
        return lhs.id < rhs.id;
    });
    return snapshot;
}

}  // namespace Engine::Audio::Plugin
