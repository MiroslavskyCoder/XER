#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <functional>

namespace Engine::Audio::Core {

// WASAPI callback type
using WASAPICallback = std::function<void(const float* input, float* output, size_t frame_count)>;

struct WASAPIDeviceInfo {
    std::string device_id;
    std::string friendly_name;
    int input_channels;
    int output_channels;
    int sample_rate;
    int buffer_size;
    bool is_default;
    bool is_active;
};

class WASAPIDevice {
public:
    WASAPIDevice();
    ~WASAPIDevice();

    // Initialization
    bool Initialize(const std::string& device_id);
    bool IsInitialized() const { return initialized_; }

    // Device information
    const WASAPIDeviceInfo& GetInfo() const { return device_info_; }
    static std::vector<WASAPIDeviceInfo> EnumerateDevices(bool render = true);
    static std::vector<WASAPIDeviceInfo> EnumerateRecordingDevices();

    // Stream operations
    bool Start();
    bool Stop();
    bool IsRunning() const { return is_running_; }

    // Configuration
    bool SetBufferSize(int frames);
    bool SetSampleRate(int sample_rate);
    bool SetChannelCount(int input_ch, int output_ch);
    bool SetExclusiveMode(bool exclusive);

    // Callback
    void SetAudioCallback(WASAPICallback callback);

    // Status & Performance
    int GetLatency() const { return latency_frames_; }
    double GetCPULoad() const { return cpu_load_; }
    bool SupportExclusive() const { return supports_exclusive_; }

private:
    WASAPIDeviceInfo device_info_;
    bool initialized_;
    bool is_running_;
    int latency_frames_;
    double cpu_load_;
    bool supports_exclusive_;
    WASAPICallback audio_callback_;

    void* client_;        // IAudioClient
    void* render_client_; // IAudioRenderClient
};

}  // namespace Engine::Audio::Core
