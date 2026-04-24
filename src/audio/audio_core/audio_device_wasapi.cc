#include "audio_device_wasapi.h"

namespace Engine::Audio::Core {

WASAPIDevice::WASAPIDevice()
    : initialized_(false), is_running_(false), latency_frames_(0), cpu_load_(0.0),
      supports_exclusive_(false), client_(nullptr), render_client_(nullptr) {
    device_info_ = WASAPIDeviceInfo{"", "", 0, 0, 44100, 512, false, false};
}

WASAPIDevice::~WASAPIDevice() {
    if (is_running_) {
        Stop();
    }
}

bool WASAPIDevice::Initialize(const std::string& device_id) {
#ifdef _WIN32
    device_info_.device_id = device_id;
    device_info_.friendly_name = "WASAPI Device";
    device_info_.input_channels = 2;
    device_info_.output_channels = 2;
    device_info_.sample_rate = 44100;
    device_info_.buffer_size = 512;
    device_info_.is_default = (device_id == "default");
    device_info_.is_active = true;
    
    supports_exclusive_ = true;
    initialized_ = true;
    return true;
#else
    return false;
#endif
}

bool WASAPIDevice::IsInitialized() const {
    return initialized_;
}

std::vector<WASAPIDeviceInfo> WASAPIDevice::EnumerateDevices(bool render) {
    std::vector<WASAPIDeviceInfo> devices;
    
#ifdef _WIN32
    // Default device
    WASAPIDeviceInfo default_dev{
        "default",
        render ? "Default Playback" : "Default Recording",
        render ? 0 : 2,
        render ? 2 : 0,
        44100,
        512,
        true,
        true
    };
    devices.push_back(default_dev);
#endif
    
    return devices;
}

std::vector<WASAPIDeviceInfo> WASAPIDevice::EnumerateRecordingDevices() {
    return EnumerateDevices(false);
}

bool WASAPIDevice::Start() {
    if (!initialized_) return false;
    
    is_running_ = true;
    return true;
}

bool WASAPIDevice::Stop() {
    is_running_ = false;
    return true;
}

bool WASAPIDevice::SetBufferSize(int frames) {
    device_info_.buffer_size = frames;
    return true;
}

bool WASAPIDevice::SetSampleRate(int sample_rate) {
    device_info_.sample_rate = sample_rate;
    return true;
}

bool WASAPIDevice::SetChannelCount(int input_ch, int output_ch) {
    device_info_.input_channels = input_ch;
    device_info_.output_channels = output_ch;
    return true;
}

bool WASAPIDevice::SetExclusiveMode(bool exclusive) {
    return supports_exclusive_ && exclusive;
}

void WASAPIDevice::SetAudioCallback(WASAPICallback callback) {
    audio_callback_ = callback;
}

int WASAPIDevice::GetLatency() const {
    return latency_frames_;
}

double WASAPIDevice::GetCPULoad() const {
    return cpu_load_;
}

bool WASAPIDevice::SupportExclusive() const {
    return supports_exclusive_;
}

}  // namespace Engine::Audio::Core
