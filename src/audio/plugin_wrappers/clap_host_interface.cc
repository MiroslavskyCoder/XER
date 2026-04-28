#include "clap_host_interface.h"

namespace Engine::Audio::Plugin {

bool ClapHostInterface::Initialize(double sample_rate, uint32_t max_block_size) {
    if (sample_rate <= 0.0 || max_block_size == 0) {
        return false;
    }
    AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    sample_rate_ = sample_rate;
    max_block_size_ = max_block_size;
    return builtin_host_.Initialize(sample_rate, max_block_size);
}

bool ClapHostInterface::LoadPlugin(const std::string& plugin_path) {
    AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    if (!builtin_host_.Load(plugin_path)) {
        return false;
    }
    plugin_path_ = builtin_host_.GetLoadedIdentifier();
    return true;
}

bool ClapHostInterface::Process(const float* input, float* output, uint32_t frames) {
    if (input == nullptr || output == nullptr || frames == 0 || frames > max_block_size_) {
        return false;
    }
    AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    return builtin_host_.Process(input, output, frames);
}

bool ClapHostInterface::SetParameter(uint32_t id, float value) {
	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	return builtin_host_.SetParameter(id, value);
}

bool ClapHostInterface::GetParameter(uint32_t id, float* value) const {
	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	return builtin_host_.GetParameter(id, value);
}

std::vector<PluginParameterInfo> ClapHostInterface::GetParameters() const {
	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	return builtin_host_.GetParameters();
}

std::string ClapHostInterface::GetLoadedPluginId() const {
	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	return plugin_path_;
}

}  // namespace Engine::Audio::Plugin
