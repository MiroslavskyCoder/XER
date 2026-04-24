#include "audio_interleave_processor.h"

#include <cstring>
#include <algorithm>

namespace Engine::Audio::Core {

AudioInterleaveProcessor::AudioInterleaveProcessor() {}

AudioInterleaveProcessor::~AudioInterleaveProcessor() {}

bool AudioInterleaveProcessor::Interleave(
    const float* const* planar_data,
    int channel_count,
    size_t frame_count,
    float* interleaved_output) {
    
    if (!planar_data || !interleaved_output || channel_count <= 0 || frame_count == 0) {
        return false;
    }

    for (size_t frame = 0; frame < frame_count; ++frame) {
        for (int ch = 0; ch < channel_count; ++ch) {
            interleaved_output[frame * channel_count + ch] = planar_data[ch][frame];
        }
    }
    
    return true;
}

bool AudioInterleaveProcessor::Deinterleave(
    const float* interleaved_data,
    int channel_count,
    size_t frame_count,
    float* const* planar_output) {
    
    if (!interleaved_data || !planar_output || channel_count <= 0 || frame_count == 0) {
        return false;
    }

    for (size_t frame = 0; frame < frame_count; ++frame) {
        for (int ch = 0; ch < channel_count; ++ch) {
            planar_output[ch][frame] = interleaved_data[frame * channel_count + ch];
        }
    }
    
    return true;
}

bool AudioInterleaveProcessor::InterleavePlanar(
    std::vector<std::vector<float>>& channels,
    size_t frame_count) {
    
    if (channels.empty() || frame_count == 0) {
        return false;
    }

    int channel_count = static_cast<int>(channels.size());
    size_t total_samples = channel_count * frame_count;
    
    // Create interleaved buffer
    std::vector<float> temp(total_samples);
    
    // Pointers for Interleave call
    std::vector<const float*> ptrs(channel_count);
    for (int i = 0; i < channel_count; ++i) {
        ptrs[i] = channels[i].data();
    }
    
    if (!Interleave(ptrs.data(), channel_count, frame_count, temp.data())) {
        return false;
    }
    
    // Replace all channels with single interleaved vector
    channels.clear();
    channels.push_back(std::move(temp));
    
    return true;
}

bool AudioInterleaveProcessor::DeinterleavePlanar(
    const float* interleaved,
    int channel_count,
    size_t frame_count,
    std::vector<std::vector<float>>& output_channels) {
    
    if (!interleaved || channel_count <= 0 || frame_count == 0) {
        return false;
    }

    output_channels.clear();
    output_channels.resize(channel_count);
    
    for (int ch = 0; ch < channel_count; ++ch) {
        output_channels[ch].resize(frame_count);
    }

    // Create pointers for Deinterleave call
    std::vector<float*> ptrs(channel_count);
    for (int i = 0; i < channel_count; ++i) {
        ptrs[i] = output_channels[i].data();
    }

    return Deinterleave(interleaved, channel_count, frame_count, ptrs.data());
}

bool AudioInterleaveProcessor::DuplicateChannel(
    const float* source,
    float* dest,
    size_t frame_count) {
    
    if (!source || !dest) return false;
    
    std::memcpy(dest, source, frame_count * sizeof(float));
    return true;
}

bool AudioInterleaveProcessor::MixChannels(
    const float* const* inputs,
    int input_channels,
    float* output,
    size_t frame_count) {
    
    if (!inputs || !output || input_channels <= 0 || frame_count == 0) {
        return false;
    }

    std::fill(output, output + frame_count, 0.0f);
    
    float scale = 1.0f / input_channels;
    for (int ch = 0; ch < input_channels; ++ch) {
        for (size_t frame = 0; frame < frame_count; ++frame) {
            output[frame] += inputs[ch][frame] * scale;
        }
    }
    
    return true;
}

}  // namespace Engine::Audio::Core
