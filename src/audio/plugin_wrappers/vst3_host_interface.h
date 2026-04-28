#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "async_io/log_and_debug/io_perf_counter.h"
#include "plugin_builtin_host.h"

namespace Engine::Audio::Plugin {

class Vst3HostInterface {
public:
	enum class BackendMode {
		kNone,
		kBuiltin,
		kUnsupportedExternal,
	};

    bool Initialize(double sample_rate, uint32_t max_block_size);
    bool LoadPlugin(const std::string& plugin_path);
    bool Process(const float* input, float* output, uint32_t frames);
    bool SetParameter(uint32_t id, float value);
    bool GetParameter(uint32_t id, float* value) const;
    std::vector<PluginParameterInfo> GetParameters() const;
    std::string GetLoadedPluginId() const;
	std::string GetBackendMode() const;

private:
    AsyncIO::IO::LogDebug::PerformanceCounter perf_counter_;
    BuiltinPluginHost builtin_host_;
    double sample_rate_ = 44100.0;
    uint32_t max_block_size_ = 512;
    std::string plugin_path_;
	BackendMode backend_mode_ = BackendMode::kNone;
};

}  // namespace Engine::Audio::Plugin
