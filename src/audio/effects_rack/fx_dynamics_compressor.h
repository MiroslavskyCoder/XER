#pragma once

#include <cstddef>
#include <string>

#include "async_io/sync_primitives/mutex_wrapper.h"

namespace Engine::Audio::FX {

class DynamicsCompressor {
public:
	DynamicsCompressor();
	~DynamicsCompressor();

	void SetThresholdDb(float threshold_db);
	void SetRatio(float ratio);
	void SetAttackMs(float attack_ms);
	void SetReleaseMs(float release_ms);
	bool ProcessBlock(const float* input, size_t frame_count, float* output);

	std::string GetReport() const;

private:
	IO::Sync::MutexWrapper mutex_;
	float threshold_db_;
	float ratio_;
	float attack_ms_;
	float release_ms_;
	float envelope_;
};

}  // namespace Engine::Audio::FX
