
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "async_io/async_buffer_pool.h"

namespace Engine::Audio::DSP {

class CircularDelayLine {
public:
	CircularDelayLine();
	~CircularDelayLine();

	bool Initialize(size_t max_delay_samples);
	void SetDelaySamples(size_t delay_samples);
	void Reset();

	float Process(float sample);
	std::string GetReport() const;

private:
	size_t max_delay_samples_;
	size_t delay_samples_;
	size_t write_index_;
	std::vector<float> buffer_;
	IO::AsyncIO::AsyncBufferPool buffer_pool_;
};

}  // namespace Engine::Audio::DSP
