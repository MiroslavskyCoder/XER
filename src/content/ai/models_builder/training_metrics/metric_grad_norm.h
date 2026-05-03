#pragma once

#include <cstddef>
#include <vector>

namespace Engine::ModelsBuilder::TrainingMetrics {

class GradientNormMetric {
 public:
	void Reset();
	void AddNorm(float norm_value);
	void AddGradient(const std::vector<float>& gradient_values);

	float Latest() const;
	float Max() const;
	float Mean() const;
	size_t Count() const { return norms_.size(); }

 private:
	std::vector<float> norms_;
};

}  // namespace Engine::ModelsBuilder::TrainingMetrics

