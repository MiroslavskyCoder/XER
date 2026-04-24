#include "fx_dynamics_compressor.h"

#include <algorithm>
#include <cmath>

namespace Engine::Audio::FX {

namespace {
float DbToLinear(float db_value) {
	return std::pow(10.0f, db_value / 20.0f);
}

float LinearToDb(float linear) {
	return 20.0f * std::log10(std::max(linear, 1e-9f));
}
}

DynamicsCompressor::DynamicsCompressor()
	: mutex_("dynamics_compressor"),
	  threshold_db_(-18.0f),
	  ratio_(4.0f),
	  attack_ms_(10.0f),
	  release_ms_(120.0f),
	  envelope_(0.0f) {}

DynamicsCompressor::~DynamicsCompressor() = default;

void DynamicsCompressor::SetThresholdDb(float threshold_db) {
	AIToolsXPro::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	threshold_db_ = threshold_db;
}

void DynamicsCompressor::SetRatio(float ratio) {
	AIToolsXPro::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	ratio_ = std::max(1.0f, ratio);
}

void DynamicsCompressor::SetAttackMs(float attack_ms) {
	AIToolsXPro::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	attack_ms_ = std::max(0.1f, attack_ms);
}

void DynamicsCompressor::SetReleaseMs(float release_ms) {
	AIToolsXPro::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	release_ms_ = std::max(1.0f, release_ms);
}

bool DynamicsCompressor::ProcessBlock(const float* input, size_t frame_count, float* output) {
	if (input == nullptr || output == nullptr) {
		return false;
	}

	AIToolsXPro::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	for (size_t i = 0; i < frame_count; ++i) {
		const float sample = input[i];
		const float abs_sample = std::abs(sample);
		envelope_ = std::max(abs_sample, envelope_ * 0.995f);

		const float input_db = LinearToDb(envelope_);
		float gain_reduction_db = 0.0f;
		if (input_db > threshold_db_) {
			const float over_db = input_db - threshold_db_;
			gain_reduction_db = over_db - (over_db / ratio_);
		}

		const float gain = DbToLinear(-gain_reduction_db);
		output[i] = sample * gain;
	}
	return true;
}

std::string DynamicsCompressor::GetReport() const {
	return "Compressor: th=" + std::to_string(threshold_db_) +
		", ratio=" + std::to_string(ratio_) +
		", attack=" + std::to_string(attack_ms_) +
		", release=" + std::to_string(release_ms_);
}

}  // namespace Engine::Audio::FX
