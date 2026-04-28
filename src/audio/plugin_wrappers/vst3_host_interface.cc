#include "vst3_host_interface.h"

namespace Engine::Audio::Plugin {

bool Vst3HostInterface::Initialize(double sample_rate, uint32_t max_block_size) {
    if (sample_rate <= 0.0 || max_block_size == 0) {
        return false;
    }
    sample_rate_ = sample_rate;
    max_block_size_ = max_block_size;
    perf_counter_.Enable();
    return builtin_host_.Initialize(sample_rate, max_block_size);
}

bool Vst3HostInterface::LoadPlugin(const std::string& plugin_path) {
    if (!builtin_host_.Load(plugin_path)) {
        return false;
    }
    plugin_path_ = builtin_host_.GetLoadedIdentifier();
    return true;
}

bool Vst3HostInterface::Process(const float* input, float* output, uint32_t frames) {
    if (input == nullptr || output == nullptr || frames == 0 || frames > max_block_size_) {
        return false;
    }
    perf_counter_.StartCounter("vst3_process");
    const bool ok = builtin_host_.Process(input, output, frames);
    perf_counter_.StopCounter("vst3_process");
    return ok;
}

bool Vst3HostInterface::SetParameter(uint32_t id, float value) {
	return builtin_host_.SetParameter(id, value);
}

bool Vst3HostInterface::GetParameter(uint32_t id, float* value) const {
	return builtin_host_.GetParameter(id, value);
}

std::vector<PluginParameterInfo> Vst3HostInterface::GetParameters() const {
	return builtin_host_.GetParameters();
}

std::string Vst3HostInterface::GetLoadedPluginId() const {
	return plugin_path_;
}

}  // namespace Engine::Audio::Plugin
