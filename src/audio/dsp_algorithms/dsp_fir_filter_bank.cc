#include "dsp_fir_filter_bank.h"

#include <algorithm>
#include <future>
#include <thread>

#if defined(__AVX2__) || defined(__SSE2__)
#include <immintrin.h>
#endif

namespace Engine::Audio::DSP {

std::string FIRFilterBank::GetMemoryStats() const {
	size_t filters_bytes = 0;
	for (const auto& f : filters_) filters_bytes += f.size() * sizeof(float);
	size_t states_bytes = 0;
	for (const auto& s : states_) states_bytes += s.size() * sizeof(float);
	std::string stats;
	stats += "Filters: " + std::to_string(filters_bytes) + " bytes\n";
	stats += "States: " + std::to_string(states_bytes) + " bytes\n";
	return stats;
}

FIRFilterBank::FIRFilterBank()
	: mutex_("fir_filter_bank") {}

FIRFilterBank::~FIRFilterBank() = default;

void FIRFilterBank::Clear() {
	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	filters_.clear();
	states_.clear();
}

bool FIRFilterBank::AddFilter(const std::vector<float>& taps) {
	if (taps.empty()) {
		return false;
	}

	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	filters_.push_back(taps);
	states_.push_back(std::vector<float>(taps.size(), 0.0f));
	return true;
}

bool FIRFilterBank::ProcessSample(float input, std::vector<float>& outputs) {
	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	outputs.assign(filters_.size(), 0.0f);

	const size_t hardware_threads = std::max<size_t>(1, std::thread::hardware_concurrency());
	const size_t num_threads = std::min(hardware_threads, filters_.size());
	if (filters_.size() < 8 || num_threads < 2) {
		for (size_t filter_index = 0; filter_index < filters_.size(); ++filter_index) {
			auto& state = states_[filter_index];
			const auto& taps = filters_[filter_index];
			if (!state.empty()) {
				std::move_backward(state.begin(), state.end() - 1, state.end());
				state[0] = input;
			}
			float y = 0.0f;
#if defined(__AVX2__)
			size_t tap = 0;
			for (; tap + 7 < taps.size(); tap += 8) {
				__m256 t = _mm256_loadu_ps(&taps[tap]);
				__m256 s = _mm256_loadu_ps(&state[tap]);
				__m256 mul = _mm256_mul_ps(t, s);
				float sum[8];
				_mm256_storeu_ps(sum, mul);
				for (int j = 0; j < 8; ++j) y += sum[j];
			}
			for (; tap < taps.size(); ++tap) {
				y += taps[tap] * state[tap];
			}
#elif defined(__SSE2__)
			size_t tap = 0;
			for (; tap + 3 < taps.size(); tap += 4) {
				__m128 t = _mm_loadu_ps(&taps[tap]);
				__m128 s = _mm_loadu_ps(&state[tap]);
				__m128 mul = _mm_mul_ps(t, s);
				float sum[4];
				_mm_storeu_ps(sum, mul);
				for (int j = 0; j < 4; ++j) y += sum[j];
			}
			for (; tap < taps.size(); ++tap) {
				y += taps[tap] * state[tap];
			}
#else
			for (size_t tap = 0; tap < taps.size(); ++tap) {
				y += taps[tap] * state[tap];
			}
#endif
			outputs[filter_index] = y;
		}
		return true;
	}
	std::vector<std::thread> threads(num_threads);
	auto worker = [&](size_t thread_idx) {
		size_t chunk = filters_.size() / num_threads;
		size_t start = thread_idx * chunk;
		size_t end = (thread_idx == num_threads - 1) ? filters_.size() : start + chunk;
		for (size_t filter_index = start; filter_index < end; ++filter_index) {
			auto& state = states_[filter_index];
			const auto& taps = filters_[filter_index];
			if (!state.empty()) {
				std::move_backward(state.begin(), state.end() - 1, state.end());
				state[0] = input;
			}
			float y = 0.0f;
#if defined(__AVX2__)
			size_t tap = 0;
			for (; tap + 7 < taps.size(); tap += 8) {
				__m256 t = _mm256_loadu_ps(&taps[tap]);
				__m256 s = _mm256_loadu_ps(&state[tap]);
				__m256 mul = _mm256_mul_ps(t, s);
				float sum[8];
				_mm256_storeu_ps(sum, mul);
				for (int j = 0; j < 8; ++j) y += sum[j];
			}
			for (; tap < taps.size(); ++tap) {
				y += taps[tap] * state[tap];
			}
#elif defined(__SSE2__)
			size_t tap = 0;
			for (; tap + 3 < taps.size(); tap += 4) {
				__m128 t = _mm_loadu_ps(&taps[tap]);
				__m128 s = _mm_loadu_ps(&state[tap]);
				__m128 mul = _mm_mul_ps(t, s);
				float sum[4];
				_mm_storeu_ps(sum, mul);
				for (int j = 0; j < 4; ++j) y += sum[j];
			}
			for (; tap < taps.size(); ++tap) {
				y += taps[tap] * state[tap];
			}
#else
			for (size_t tap = 0; tap < taps.size(); ++tap) {
				y += taps[tap] * state[tap];
			}
#endif
			outputs[filter_index] = y;
		}
	};
	for (size_t t = 0; t < num_threads; ++t) {
		threads[t] = std::thread(worker, t);
	}
	for (auto& th : threads) th.join();
	return true;
}

std::string FIRFilterBank::GetReport() const {
	return "FIRBank: filters=" + std::to_string(filters_.size());
}


std::future<bool> FIRFilterBank::ProcessSampleAsync(float input, std::vector<float>& outputs) {
	return std::async(std::launch::async, [this, input, &outputs]() {
		return this->ProcessSample(input, outputs);
	});
}

}  // namespace Engine::Audio::DSP
