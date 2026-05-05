/**
 * This file is part of XER, Device open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#include "content/device/cast/aura/aura_lc3_decoder.h"

#include <algorithm>
#include <cstring>
#include <dlfcn.h>

namespace device::cast::aura {

// liblc3 PCM format constants (LC3_PCM_FORMAT_FLOAT = 3).
static constexpr int kLc3PcmFormatFloat = 3;

AuraLc3Decoder::AuraLc3Decoder() = default;

AuraLc3Decoder::~AuraLc3Decoder() {
    if (lc3_lib_) { dlclose(lc3_lib_); lc3_lib_ = nullptr; }
}

bool AuraLc3Decoder::LoadLc3Library() {
    // Try versioned and unversioned names (apt: liblc3-dev).
    static const char* kNames[] = { "liblc3.so.1", "liblc3.so", nullptr };
    for (int i = 0; kNames[i]; ++i) {
        lc3_lib_ = dlopen(kNames[i], RTLD_LAZY | RTLD_LOCAL);
        if (lc3_lib_) break;
    }
    if (!lc3_lib_) return false;

    pfn_lc3_frame_samples_ = reinterpret_cast<PfnLc3FrameSamples>(
        dlsym(lc3_lib_, "lc3_frame_samples"));
    pfn_lc3_decoder_size_  = reinterpret_cast<PfnLc3DecoderSize>(
        dlsym(lc3_lib_, "lc3_decoder_size"));
    pfn_lc3_setup_decoder_ = reinterpret_cast<PfnLc3SetupDecoder>(
        dlsym(lc3_lib_, "lc3_setup_decoder"));
    pfn_lc3_decode_        = reinterpret_cast<PfnLc3Decode>(
        dlsym(lc3_lib_, "lc3_decode"));

    if (!pfn_lc3_frame_samples_ || !pfn_lc3_decoder_size_ ||
        !pfn_lc3_setup_decoder_ || !pfn_lc3_decode_) {
        dlclose(lc3_lib_); lc3_lib_ = nullptr; return false;
    }
    return true;
}

bool AuraLc3Decoder::Initialize(const Lc3Params& p) {
    Reset();
    params_ = p;

    const bool has_lib = LoadLc3Library();

    if (has_lib) {
        // Query per-frame PCM sample count.
        pcm_samples_per_frame_ = pfn_lc3_frame_samples_(
            static_cast<int>(p.frame_duration_us),
            static_cast<int>(p.sampling_freq_hz));
        if (pcm_samples_per_frame_ <= 0) {
            dlclose(lc3_lib_); lc3_lib_ = nullptr;
            initialized_ = true; return true;  // fallback mode
        }

        // Allocate per-channel decoder memory.
        const unsigned sz = pfn_lc3_decoder_size_(
            static_cast<int>(p.frame_duration_us),
            static_cast<int>(p.sampling_freq_hz));

        decoder_mem_.resize(p.channel_count);
        decoders_.resize(p.channel_count);
        for (int ch = 0; ch < p.channel_count; ++ch) {
            decoder_mem_[ch].assign(sz, 0);
            decoders_[ch] = pfn_lc3_setup_decoder_(
                static_cast<int>(p.frame_duration_us),
                static_cast<int>(p.sampling_freq_hz),
                static_cast<int>(p.sampling_freq_hz),  // output rate == input
                decoder_mem_[ch].data());
        }
    } else {
        // Passthrough: estimate PCM frames from bit-rate.
        // PCM samples = frame_duration_us * sample_rate / 1e6
        pcm_samples_per_frame_ = static_cast<int>(
            static_cast<uint64_t>(p.frame_duration_us) * p.sampling_freq_hz / 1'000'000ULL);
    }

    initialized_ = true;
    return true;
}

void AuraLc3Decoder::Reset() {
    decoder_mem_.clear();
    decoders_.clear();
    pcm_samples_per_frame_ = 0;
    initialized_ = false;
}

Lc3DecodedFrame AuraLc3Decoder::SilenceFrame(uint8_t channels) const {
    Lc3DecodedFrame out;
    out.sample_rate  = params_.sampling_freq_hz;
    out.channels     = channels;
    out.frame_count  = static_cast<size_t>(pcm_samples_per_frame_);
    out.planar.resize(channels,
        std::vector<float>(static_cast<size_t>(pcm_samples_per_frame_), 0.f));
    return out;
}

Lc3DecodedFrame AuraLc3Decoder::Decode(uint8_t bis_index,
                                         const uint8_t* data, size_t len) {
    if (!initialized_ || bis_index >= params_.channel_count)
        return SilenceFrame(1);

    Lc3DecodedFrame out;
    out.sample_rate = params_.sampling_freq_hz;
    out.channels    = 1;
    out.frame_count = static_cast<size_t>(pcm_samples_per_frame_);
    out.planar.resize(1, std::vector<float>(
        static_cast<size_t>(pcm_samples_per_frame_), 0.f));

    if (lc3_lib_ && bis_index < decoders_.size() && decoders_[bis_index]) {
        const int rc = pfn_lc3_decode_(
            decoders_[bis_index],
            data, static_cast<int>(len),
            kLc3PcmFormatFloat,
            out.planar[0].data(),
            pcm_samples_per_frame_);
        if (rc != 0) {
            // Decode error → output silence (PLC would go here).
            std::fill(out.planar[0].begin(), out.planar[0].end(), 0.f);
        }
    }
    // Without liblc3, planar[0] remains zeroed (silence passthrough).
    return out;
}

Lc3DecodedFrame AuraLc3Decoder::DecodeStereo(const uint8_t* left,
                                               const uint8_t* right,
                                               size_t frame_bytes) {
    auto l = Decode(0, left, frame_bytes);
    auto r = Decode(1, right, frame_bytes);

    Lc3DecodedFrame out;
    out.sample_rate = params_.sampling_freq_hz;
    out.channels    = 2;
    out.frame_count = l.frame_count;
    out.planar.push_back(std::move(l.planar[0]));
    out.planar.push_back(std::move(r.planar[0]));
    return out;
}

}  // namespace device::cast::aura
