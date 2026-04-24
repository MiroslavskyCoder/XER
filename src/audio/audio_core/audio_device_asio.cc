#include "audio_device_asio.h"

namespace Engine::Audio::Core {

ASIODevice::ASIODevice()
    : initialized_(false), is_running_(false), latency_frames_(0), asio_driver_(nullptr) {
    device_info_ = ASIODeviceInfo{0, "", 0, 0, 44100, 512, false};
}

ASIODevice::~ASIODevice() {
    if (is_running_) {
        Stop();
    }
}

bool ASIODevice::Initialize(int device_id) {
#if defined(_WIN32) && __has_include(<asiosdk/asio.h>)
    // ASIO SDK initialization would go here
    device_info_.device_id = device_id;
    device_info_.name = "ASIO Device " + std::to_string(device_id);
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

bool ASIODevice::IsInitialized() const {
    return initialized_;
}

std::vector<int> ASIODevice::GetAvailableSampleRates() const {
    return {44100, 48000, 96000, 192000};
}

bool ASIODevice::Start() {
    if (!initialized_) return false;
    
    is_running_ = true;
    return true;
}

bool ASIODevice::Stop() {
    is_running_ = false;
    return true;
}

bool ASIODevice::SetBufferSize(int frames) {
    device_info_.buffer_size = frames;
    return true;
}

bool ASIODevice::SetSampleRate(int sample_rate) {
    device_info_.sample_rate = sample_rate;
    return true;
}

bool ASIODevice::SetChannelCount(int input_ch, int output_ch) {
    device_info_.input_channels = input_ch;
    device_info_.output_channels = output_ch;
    return true;
}

void ASIODevice::SetBufferCallback(ASIOBufferCallback callback) {
    buffer_callback_ = callback;
}

int ASIODevice::GetLatency() const {
    return latency_frames_;
}

}  // namespace Engine::Audio::Core
