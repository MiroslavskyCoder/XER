#include "audio_device_coreaudio.h"

namespace Engine::Audio::Core {

CoreAudioDevice::CoreAudioDevice()
    : initialized_(false), is_running_(false), latency_frames_(0), 
      audio_unit_(nullptr), audio_graph_(nullptr) {
    device_info_ = CoreAudioDeviceInfo{0, "", 0, 0, 44100, 512, false};
}

CoreAudioDevice::~CoreAudioDevice() {
    if (is_running_) {
        Stop();
    }
}

bool CoreAudioDevice::Initialize(uint32_t device_id) {
#if defined(__APPLE__) && __has_include(<CoreAudio/CoreAudio.h>)
    // CoreAudio initialization would go here
    device_info_.device_id = device_id;
    device_info_.name = "CoreAudio Device " + std::to_string(device_id);
    device_info_.input_channels = 2;
    device_info_.output_channels = 2;
    device_info_.sample_rate = 44100;
    device_info_.buffer_size = 512;
    device_info_.is_active = true;
    
    initialized_ = true;
    return true;
#else
    return false;
#endif
}

bool CoreAudioDevice::IsInitialized() const {
    return initialized_;
}

std::vector<CoreAudioDeviceInfo> CoreAudioDevice::EnumerateDevices() {
    std::vector<CoreAudioDeviceInfo> devices;
    
#if defined(__APPLE__) && __has_include(<CoreAudio/CoreAudio.h>)
    // Device enumeration would go here
    CoreAudioDeviceInfo default_dev{0, "Default Device", 2, 2, 44100, 512, true};
    devices.push_back(default_dev);
#endif
    
    return devices;
}

bool CoreAudioDevice::Start() {
    if (!initialized_) return false;
    
    is_running_ = true;
    return true;
}

bool CoreAudioDevice::Stop() {
    is_running_ = false;
    return true;
}

bool CoreAudioDevice::SetBufferSize(int frames) {
    device_info_.buffer_size = frames;
    return true;
}

bool CoreAudioDevice::SetSampleRate(int sample_rate) {
    device_info_.sample_rate = sample_rate;
    return true;
}

bool CoreAudioDevice::SetChannelCount(int input_ch, int output_ch) {
    device_info_.input_channels = input_ch;
    device_info_.output_channels = output_ch;
    return true;
}

void CoreAudioDevice::SetAudioCallback(CoreAudioCallback callback) {
    audio_callback_ = callback;
}

int CoreAudioDevice::GetLatency() const {
    return latency_frames_;
}

std::string CoreAudioDevice::GetDeviceName() const {
    return device_info_.name;
}

}  // namespace Engine::Audio::Core
