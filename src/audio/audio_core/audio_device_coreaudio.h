#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <functional>

namespace Engine::Audio::Core {

// CoreAudio callback type
using CoreAudioCallback = std::function<void(const float* input, float* output, size_t frame_count)>;

struct CoreAudioDeviceInfo {
    uint32_t device_id;
    std::string name;
    int input_channels;
    int output_channels;
    int sample_rate;
    int buffer_size;
    bool is_active;
};

class CoreAudioDevice {
public:
    CoreAudioDevice();
    ~CoreAudioDevice();

    // Initialization
    bool Initialize(uint32_t device_id);
    bool IsInitialized() const { return initialized_; }

    // Device information
    const CoreAudioDeviceInfo& GetInfo() const { return device_info_; }
    static std::vector<CoreAudioDeviceInfo> EnumerateDevices();

    // Stream operations
    bool Start();
    bool Stop();
    bool IsRunning() const { return is_running_; }

    // Configuration
    bool SetBufferSize(int frames);
    bool SetSampleRate(int sample_rate);
    bool SetChannelCount(int input_ch, int output_ch);

    // Callback
    void SetAudioCallback(CoreAudioCallback callback);

    // Status & Info
    int GetLatency() const { return latency_frames_; }
    std::string GetDeviceName() const { return device_info_.name; }

private:
    CoreAudioDeviceInfo device_info_;
    bool initialized_;
    bool is_running_;
    int latency_frames_;
    CoreAudioCallback audio_callback_;

    void* audio_unit_;  // AUNode/AudioUnit handle
    void* audio_graph_;  // AUGraph handle
};

}  // namespace Engine::Audio::Core
