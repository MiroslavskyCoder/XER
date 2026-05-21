#include "fx_reverb_algorithmic.h"

#include <algorithm>
#include <future>

namespace Engine::Audio::FX {

std::string ReverbAlgorithmic::GetMemoryStats() const {
	std::string stats;
	stats += "CombA: " + std::to_string(comb_a_.GetReport().size()) + " bytes (report string)\n";
	stats += "CombB: " + std::to_string(comb_b_.GetReport().size()) + " bytes (report string)\n";
	stats += "Allpass: " + std::to_string(allpass_.GetReport().size()) + " bytes (report string)\n";
	return stats;
}

ReverbAlgorithmic::ReverbAlgorithmic()
	: sample_rate_(44100.0f),
	  room_size_(0.6f),
	  damping_(0.3f),
	  mix_(0.25f) {
	perf_counter_.Enable();
}

ReverbAlgorithmic::~ReverbAlgorithmic() = default;

bool ReverbAlgorithmic::Initialize(float sample_rate, size_t max_delay_samples) {
	if (sample_rate <= 0.0f || max_delay_samples < 32) {
		return false;
	}
	sample_rate_ = sample_rate;
	const bool ok_a = comb_a_.Initialize(max_delay_samples);
	const bool ok_b = comb_b_.Initialize(max_delay_samples);
	const bool ok_ap = allpass_.Initialize(max_delay_samples);
	return ok_a && ok_b && ok_ap;
}

void ReverbAlgorithmic::SetRoomSize(float room_size) {
	room_size_ = std::clamp(room_size, 0.1f, 1.0f);
}

void ReverbAlgorithmic::SetDamping(float damping) {
	damping_ = std::clamp(damping, 0.0f, 0.99f);
}

void ReverbAlgorithmic::SetMix(float mix) {
	mix_ = std::clamp(mix, 0.0f, 1.0f);
}

bool ReverbAlgorithmic::ProcessBlock(const float* input, size_t frame_count, float* output) {
	if (input == nullptr || output == nullptr) {
		return false;
	}
	perf_counter_.StartCounter("fx_reverb_algo");
	const size_t comb_delay_a = static_cast<size_t>(room_size_ * 1400.0f + 50.0f);
	const size_t comb_delay_b = static_cast<size_t>(room_size_ * 1700.0f + 70.0f);
	const size_t allpass_delay = static_cast<size_t>(room_size_ * 300.0f + 20.0f);
	comb_a_.SetDelaySamples(comb_delay_a);
	comb_b_.SetDelaySamples(comb_delay_b);
	allpass_.SetDelaySamples(allpass_delay);

	size_t n = 0;
	// SIMD-обработка для двух comb и одного allpass
#if defined(__AVX2__)
	for (; n + 7 < frame_count; n += 8) {
		float wet[8];
		for (int j = 0; j < 8; ++j) wet[j] = 0.0f;
		for (int j = 0; j < 8; ++j) {
			wet[j] += comb_a_.Process(input[n + j]);
			wet[j] += comb_b_.Process(input[n + j]);
			wet[j] = allpass_.Process(wet[j]);
		}
		for (int j = 0; j < 8; ++j) {
			output[n + j] = input[n + j] * (1.0f - mix_) + wet[j] * mix_;
		}
	}
#endif
#if defined(__SSE2__)
	for (; n + 3 < frame_count; n += 4) {
		float wet[4];
		for (int j = 0; j < 4; ++j) wet[j] = 0.0f;
		for (int j = 0; j < 4; ++j) {
			wet[j] += comb_a_.Process(input[n + j]);
			wet[j] += comb_b_.Process(input[n + j]);
			wet[j] = allpass_.Process(wet[j]);
		}
		for (int j = 0; j < 4; ++j) {
			output[n + j] = input[n + j] * (1.0f - mix_) + wet[j] * mix_;
		}
	}
#endif
	for (; n < frame_count; ++n) {
		float wet = 0.0f;
		wet += comb_a_.Process(input[n]);
		wet += comb_b_.Process(input[n]);
		wet = allpass_.Process(wet);
		output[n] = input[n] * (1.0f - mix_) + wet * mix_;
	}
	perf_counter_.StopCounter("fx_reverb_algo");
	return true;
}

std::string ReverbAlgorithmic::GetReport() const {
	return "ReverbAlgo: room=" + std::to_string(room_size_) +
		", damp=" + std::to_string(damping_) +
		", mix=" + std::to_string(mix_);
}


std::future<bool> ReverbAlgorithmic::ProcessBlockAsync(const float* input, size_t frame_count, float* output) {
	return std::async(std::launch::async, [this, input, frame_count, output]() {
		return this->ProcessBlock(input, frame_count, output);
	});
}

}  // namespace Engine::Audio::FX
