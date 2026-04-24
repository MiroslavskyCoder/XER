#include "fx_dynamics_gate.h"

#include <algorithm>
#include <cmath>

namespace Engine::Audio::FX {

namespace {
float LinearToDb(float linear) {
	return 20.0f * std::log10(std::max(linear, 1e-9f));
}
}

DynamicsGate::DynamicsGate()
	: threshold_db_(-50.0f),
	  hold_samples_(256),
	  hold_counter_(0),
	  gate_open_(false) {
	perf_counter_.Enable();
}

DynamicsGate::~DynamicsGate() = default;

void DynamicsGate::SetThresholdDb(float threshold_db) {
	threshold_db_ = threshold_db;
}

void DynamicsGate::SetHoldSamples(size_t hold_samples) {
	hold_samples_ = std::max<size_t>(1, hold_samples);
}

bool DynamicsGate::ProcessBlock(const float* input, size_t frame_count, float* output) {
	if (input == nullptr || output == nullptr) {
		return false;
	}

	perf_counter_.StartCounter("fx_gate");
	for (size_t i = 0; i < frame_count; ++i) {
		const float in = input[i];
		const float level_db = LinearToDb(std::abs(in));

		if (level_db >= threshold_db_) {
			gate_open_ = true;
			hold_counter_ = hold_samples_;
		} else if (hold_counter_ > 0) {
			--hold_counter_;
		} else {
			gate_open_ = false;
		}

		output[i] = gate_open_ ? in : 0.0f;
	}
	perf_counter_.StopCounter("fx_gate");
	return true;
}

std::string DynamicsGate::GetReport() const {
	return "Gate: th=" + std::to_string(threshold_db_) +
		", hold=" + std::to_string(hold_samples_) +
		", open=" + std::to_string(gate_open_ ? 1 : 0);
}

}  // namespace Engine::Audio::FX
