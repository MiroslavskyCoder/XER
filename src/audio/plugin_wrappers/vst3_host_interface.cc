#include "vst3_host_interface.h"

#include <algorithm>
#include <cctype>

namespace {

std::string NormalizeBuiltinReference(std::string reference) {
    std::transform(reference.begin(), reference.end(), reference.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    constexpr const char* kBuiltinPrefix = "builtin://";
    if (reference.rfind(kBuiltinPrefix, 0) == 0) {
        reference.erase(0, std::char_traits<char>::length(kBuiltinPrefix));
    }
    constexpr const char* kBuiltinShortPrefix = "builtin:";
    if (reference.rfind(kBuiltinShortPrefix, 0) == 0) {
        reference.erase(0, std::char_traits<char>::length(kBuiltinShortPrefix));
    }
    return reference;
}

bool IsBuiltinPluginReference(const std::string& reference) {
    const std::string normalized = NormalizeBuiltinReference(reference);
    return normalized == "passthrough"
        || normalized == "gain"
        || normalized == "compressor"
        || normalized == "chorus"
        || normalized == "parametric_eq"
        || normalized == "eq";
}

}  // namespace

namespace Engine::Audio::Plugin {

bool Vst3HostInterface::Initialize(double sample_rate, uint32_t max_block_size) {
    if (sample_rate <= 0.0 || max_block_size == 0) {
        return false;
    }
    sample_rate_ = sample_rate;
    max_block_size_ = max_block_size;
	plugin_path_.clear();
	backend_mode_ = BackendMode::kNone;
    perf_counter_.Enable();
    return builtin_host_.Initialize(sample_rate, max_block_size);
}

bool Vst3HostInterface::LoadPlugin(const std::string& plugin_path) {
    if (!builtin_host_.Load(plugin_path)) {
		backend_mode_ = IsBuiltinPluginReference(plugin_path) ? BackendMode::kNone : BackendMode::kUnsupportedExternal;
		plugin_path_ = backend_mode_ == BackendMode::kUnsupportedExternal
			? "unsupported://vst3-external-sdk-missing"
			: std::string();
        return false;
    }
	backend_mode_ = BackendMode::kBuiltin;
    plugin_path_ = builtin_host_.GetLoadedIdentifier();
    return true;
}

bool Vst3HostInterface::Process(const float* input, float* output, uint32_t frames) {
    if (input == nullptr || output == nullptr || frames == 0 || frames > max_block_size_) {
        return false;
    }
    if (backend_mode_ != BackendMode::kBuiltin) {
        return false;
    }
    perf_counter_.StartCounter("vst3_process");
    const bool ok = builtin_host_.Process(input, output, frames);
    perf_counter_.StopCounter("vst3_process");
    return ok;
}

bool Vst3HostInterface::SetParameter(uint32_t id, float value) {
    if (backend_mode_ != BackendMode::kBuiltin) {
        return false;
    }
	return builtin_host_.SetParameter(id, value);
}

bool Vst3HostInterface::GetParameter(uint32_t id, float* value) const {
    if (backend_mode_ != BackendMode::kBuiltin) {
        return false;
    }
	return builtin_host_.GetParameter(id, value);
}

std::vector<PluginParameterInfo> Vst3HostInterface::GetParameters() const {
    if (backend_mode_ != BackendMode::kBuiltin) {
        return {};
    }
	return builtin_host_.GetParameters();
}

std::string Vst3HostInterface::GetLoadedPluginId() const {
	return plugin_path_;
}

std::string Vst3HostInterface::GetBackendMode() const {
    switch (backend_mode_) {
    case BackendMode::kBuiltin:
        return "builtin";
    case BackendMode::kUnsupportedExternal:
        return "unsupported_external";
    case BackendMode::kNone:
        return "none";
    }
    return "none";
}

}  // namespace Engine::Audio::Plugin
