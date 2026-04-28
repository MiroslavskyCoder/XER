#include "clap_host_interface.h"

namespace Engine::Audio::Plugin {

ClapHostInterface::~ClapHostInterface() = default;

bool ClapHostInterface::Initialize(double sample_rate, uint32_t max_block_size) {
    if (sample_rate <= 0.0 || max_block_size == 0) {
        return false;
    }
    AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    sample_rate_ = sample_rate;
    max_block_size_ = max_block_size;
    backend_ = Backend::kNone;
    if (external_host_ == nullptr) {
        external_host_ = std::make_unique<ClapExternalPluginHost>();
    }
    const bool builtin_ready = builtin_host_.Initialize(sample_rate, max_block_size);
    const bool external_ready = external_host_->Initialize(sample_rate, max_block_size);
    return builtin_ready && external_ready;
}

bool ClapHostInterface::LoadPlugin(const std::string& plugin_path) {
    AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    if (builtin_host_.Load(plugin_path)) {
        plugin_path_ = builtin_host_.GetLoadedIdentifier();
        backend_ = Backend::kBuiltin;
        return true;
    }
    if (external_host_ != nullptr && external_host_->Load(plugin_path)) {
        plugin_path_ = external_host_->GetLoadedIdentifier();
        backend_ = Backend::kExternal;
        return true;
    }
    backend_ = Backend::kNone;
    plugin_path_.clear();
    return false;
}

bool ClapHostInterface::Process(const float* input, float* output, uint32_t frames) {
    if (input == nullptr || output == nullptr || frames == 0 || frames > max_block_size_) {
        return false;
    }
    AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    switch (backend_) {
    case Backend::kBuiltin:
        return builtin_host_.Process(input, output, frames);
    case Backend::kExternal:
        return external_host_ != nullptr && external_host_->Process(input, output, frames);
    case Backend::kNone:
        return false;
    }
    return false;
}

bool ClapHostInterface::SetParameter(uint32_t id, float value) {
	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    switch (backend_) {
    case Backend::kBuiltin:
        return builtin_host_.SetParameter(id, value);
    case Backend::kExternal:
        return external_host_ != nullptr && external_host_->SetParameter(id, value);
    case Backend::kNone:
        return false;
    }
    return false;
}

bool ClapHostInterface::GetParameter(uint32_t id, float* value) const {
	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    switch (backend_) {
    case Backend::kBuiltin:
        return builtin_host_.GetParameter(id, value);
    case Backend::kExternal:
        return external_host_ != nullptr && external_host_->GetParameter(id, value);
    case Backend::kNone:
        return false;
    }
    return false;
}

std::vector<PluginParameterInfo> ClapHostInterface::GetParameters() const {
	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    switch (backend_) {
    case Backend::kBuiltin:
        return builtin_host_.GetParameters();
    case Backend::kExternal:
        return external_host_ != nullptr ? external_host_->GetParameters() : std::vector<PluginParameterInfo>{};
    case Backend::kNone:
        return {};
    }
    return {};
}

std::string ClapHostInterface::GetLoadedPluginId() const {
	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	return plugin_path_;
}

}  // namespace Engine::Audio::Plugin
