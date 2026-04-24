#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <functional>

namespace Engine::Audio::Core {

// ASIO callback type
using ASIOBufferCallback = std::function<void(const float* input, float* output, size_t frame_count)>;

struct ASIODeviceInfo {
    int device_id;
    std::string name;
    int input_channels;
    int output_channels;
    int sample_rate;
    int buffer_size;
    bool is_active;
};

class ASIODevice {
public:
    ASIODevice();
    ~ASIODevice();

    // Initialization
    bool Initialize(int device_id);
    bool IsInitialized() const { return initialized_; }

    // Device information
    const ASIODeviceInfo& GetInfo() const { return device_info_; }
    std::vector<int> GetAvailableSampleRates() const;

    // Stream operations
    bool Start();
    bool Stop();
    bool IsRunning() const { return is_running_; }

    // Configuration
    bool SetBufferSize(int frames);
    bool SetSampleRate(int sample_rate);
    bool SetChannelCount(int input_ch, int output_ch);

    // Callback
    void SetBufferCallback(ASIOBufferCallback callback);

    // Status
    int GetLatency() const { return latency_frames_; }

private:
    ASIODeviceInfo device_info_;
    bool initialized_;
    bool is_running_;
    int latency_frames_;
    ASIOBufferCallback buffer_callback_;

    void* asio_driver_;  // Platform-specific handle
};

}  // namespace Engine::Audio::Core
