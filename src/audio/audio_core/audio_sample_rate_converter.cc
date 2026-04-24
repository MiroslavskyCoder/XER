#include "audio_sample_rate_converter.h"

#include <cmath>
#include <algorithm>

namespace Engine::Audio::Core {

AudioSampleRateConverter::AudioSampleRateConverter(ResampleQuality quality)
    : input_rate_(0), output_rate_(0), ratio_(1.0), quality_(quality),
      initialized_(false), buffer_pos_(0.0), buffer_pool_(8192, 4) {}

AudioSampleRateConverter::~AudioSampleRateConverter() {}

bool AudioSampleRateConverter::Initialize(int input_rate, int output_rate) {
    if (input_rate <= 0 || output_rate <= 0) return false;

    input_rate_ = input_rate;
    output_rate_ = output_rate;
    ratio_ = static_cast<double>(output_rate_) / input_rate_;
    
    GenerateFilterKernel();
    initialized_ = true;
    return true;
}

bool AudioSampleRateConverter::Convert(const float* input, size_t input_frames, float* output, size_t& output_frames) {
    if (!initialized_ || !input || !output) return false;

    output_frames = static_cast<size_t>(input_frames * ratio_);
    
    for (size_t i = 0; i < output_frames; ++i) {
        double pos = i / ratio_;
        int index = static_cast<int>(pos);
        
        if (index < static_cast<int>(input_frames) - 1) {
            output[i] = InterpolateLinear(input, pos);
        } else if (index < static_cast<int>(input_frames)) {
            output[i] = input[index];
        } else {
            output[i] = 0.0f;
        }
    }

    return true;
}

bool AudioSampleRateConverter::ConvertMultiChannel(
    const float* const* input,
    int channels,
    size_t input_frames,
    float* const* output,
    size_t& output_frames) {

    if (!initialized_ || !input || !output || channels <= 0) return false;

    output_frames = static_cast<size_t>(input_frames * ratio_);

    for (int ch = 0; ch < channels; ++ch) {
        if (!Convert(input[ch], input_frames, output[ch], output_frames)) {
            return false;
        }
    }

    return true;
}

std::string AudioSampleRateConverter::GetInfo() const {
    std::string info = "SampleRateConverter: ";
    info += std::to_string(input_rate_) + "Hz -> " + std::to_string(output_rate_) + "Hz";
    return info;
}

void AudioSampleRateConverter::GenerateFilterKernel() {
    // Simple sinc windowed filter kernel for resampling
    int kernel_size = 64;
    filter_kernel_.resize(kernel_size);
    
    double cutoff = std::min(1.0 / ratio_, 1.0);
    
    for (int i = 0; i < kernel_size; ++i) {
        double x = i - kernel_size / 2.0;
        
        if (std::abs(x) < 1e-6) {
            filter_kernel_[i] = cutoff;
        } else {
            double sinc_val = std::sin(M_PI * x * cutoff) / (M_PI * x);
            // Hamming window
            double window = 0.54 - 0.46 * std::cos(2 * M_PI * i / kernel_size);
            filter_kernel_[i] = sinc_val * window;
        }
    }
}

float AudioSampleRateConverter::InterpolateLinear(const float* buffer, double pos) const {
    int index = static_cast<int>(pos);
    double frac = pos - index;
    
    return buffer[index] * (1.0f - frac) + buffer[index + 1] * frac;
}

}  // namespace Engine::Audio::Core
