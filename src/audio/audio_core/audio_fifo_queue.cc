#include "audio_fifo_queue.h"

#include <cstring>
#include <algorithm>

namespace Engine::Audio::Core {

AudioFIFOQueue::AudioFIFOQueue(size_t capacity)
    : capacity_(capacity), read_pos_(0), write_pos_(0), available_frames_(0),
      total_written_(0), total_read_(0), overflow_count_(0) {
    buffer_.resize(capacity);
}

AudioFIFOQueue::~AudioFIFOQueue() {}

size_t AudioFIFOQueue::Write(const float* data, size_t frame_count) {
    if (!data || frame_count == 0) return 0;

    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t writable = capacity_ - available_frames_;
    size_t to_write = std::min(frame_count, writable);
    
    if (to_write < frame_count) {
        overflow_count_++;
    }

    // Copy to circular buffer
    size_t write_now = std::min(to_write, capacity_ - write_pos_);
    std::memcpy(buffer_.data() + write_pos_, data, write_now * sizeof(float));
    
    if (write_now < to_write) {
        std::memcpy(buffer_.data(), data + write_now, (to_write - write_now) * sizeof(float));
    }

    write_pos_ = (write_pos_ + to_write) % capacity_;
    available_frames_ += to_write;
    total_written_ += to_write;
    
    return to_write;
}

size_t AudioFIFOQueue::Write(const float* const* data, int channels, size_t frame_count) {
    if (!data || channels <= 0 || frame_count == 0) return 0;

    // Interleave multi-channel data and write
    std::vector<float> interleaved(frame_count * channels);
    for (size_t i = 0; i < frame_count; ++i) {
        for (int ch = 0; ch < channels; ++ch) {
            interleaved[i * channels + ch] = data[ch][i];
        }
    }
    
    return Write(interleaved.data(), frame_count * channels);
}

size_t AudioFIFOQueue::Read(float* data, size_t frame_count) {
    if (!data || frame_count == 0) return 0;

    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t to_read = std::min(frame_count, available_frames_);

    // Copy from circular buffer
    size_t read_now = std::min(to_read, capacity_ - read_pos_);
    std::memcpy(data, buffer_.data() + read_pos_, read_now * sizeof(float));
    
    if (read_now < to_read) {
        std::memcpy(data + read_now, buffer_.data(), (to_read - read_now) * sizeof(float));
    }

    read_pos_ = (read_pos_ + to_read) % capacity_;
    available_frames_ -= to_read;
    total_read_ += to_read;
    
    return to_read;
}

size_t AudioFIFOQueue::Read(float* const* data, int channels, size_t frame_count) {
    if (!data || channels <= 0 || frame_count == 0) return 0;

    // Read interleaved and deinterleave
    std::vector<float> interleaved(frame_count * channels);
    size_t read_count = Read(interleaved.data(), frame_count * channels) / channels;
    
    for (size_t i = 0; i < read_count; ++i) {
        for (int ch = 0; ch < channels; ++ch) {
            data[ch][i] = interleaved[i * channels + ch];
        }
    }
    
    return read_count;
}

size_t AudioFIFOQueue::GetAvailableFrames() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return available_frames_;
}

void AudioFIFOQueue::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    read_pos_ = 0;
    write_pos_ = 0;
    available_frames_ = 0;
}

void AudioFIFOQueue::Reset() {
    Clear();
    total_written_ = 0;
    total_read_ = 0;
    overflow_count_ = 0;
}

}  // namespace Engine::Audio::Core
