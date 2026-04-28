#include "au_host_interface.h"

#include <algorithm>

namespace Engine::Audio::Plugin {

bool AuHostInterface::Initialize(double sample_rate, uint32_t max_block_size) {
    if (sample_rate <= 0.0 || max_block_size == 0) {
        return false;
    }
    AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    sample_rate_ = sample_rate;
    max_block_size_ = max_block_size;
    return builtin_host_.Initialize(sample_rate, max_block_size);
}

bool AuHostInterface::LoadComponent(const std::string& component_id) {
    AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    if (!builtin_host_.Load(component_id)) {
        return false;
    }
    component_id_ = builtin_host_.GetLoadedIdentifier();
    return true;
}

bool AuHostInterface::Process(const float* input, float* output, uint32_t frames) {
    if (input == nullptr || output == nullptr || frames == 0 || frames > max_block_size_) {
        return false;
    }
    AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    return builtin_host_.Process(input, output, frames);
}

bool AuHostInterface::SetParameter(uint32_t id, float value) {
	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	return builtin_host_.SetParameter(id, value);
}

bool AuHostInterface::GetParameter(uint32_t id, float* value) const {
	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	return builtin_host_.GetParameter(id, value);
}

std::vector<PluginParameterInfo> AuHostInterface::GetParameters() const {
	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	return builtin_host_.GetParameters();
}

std::string AuHostInterface::GetLoadedComponentId() const {
	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	return component_id_;
}

}  // namespace Engine::Audio::Plugin
