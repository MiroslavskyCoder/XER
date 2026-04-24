#include "dsp_convolution_engine.h"

#include <algorithm>

namespace Engine::Audio::DSP {

ConvolutionEngine::ConvolutionEngine()
	: buffer_pool_(65536, 2) {
	perf_counter_.Enable();
}

ConvolutionEngine::~ConvolutionEngine() = default;

bool ConvolutionEngine::SetImpulseResponse(const float* ir, size_t ir_size) {
	if (ir == nullptr || ir_size == 0) {
		return false;
	}

	impulse_response_.assign(ir, ir + ir_size);
	history_.assign(ir_size, 0.0f);
	return true;
}

bool ConvolutionEngine::ProcessBlock(const float* input, size_t frame_count, float* output) {
	if (input == nullptr || output == nullptr || frame_count == 0 || impulse_response_.empty()) {
		return false;
	}

	perf_counter_.StartCounter("convolution_block");

	auto tmp = buffer_pool_.AcquireBuffer();
	if (tmp != nullptr) {
		tmp->used_bytes = 0;
		buffer_pool_.ReleaseBuffer(tmp);
	}

	for (size_t n = 0; n < frame_count; ++n) {
		history_.insert(history_.begin(), input[n]);
		history_.pop_back();

		float acc = 0.0f;
		for (size_t k = 0; k < impulse_response_.size(); ++k) {
			acc += impulse_response_[k] * history_[k];
		}
		output[n] = acc;
	}

	perf_counter_.StopCounter("convolution_block");
	return true;
}

void ConvolutionEngine::Reset() {
	std::fill(history_.begin(), history_.end(), 0.0f);
}

std::string ConvolutionEngine::GetReport() const {
	return "Convolution: ir_size=" + std::to_string(impulse_response_.size());
}

}  // namespace Engine::Audio::DSP
