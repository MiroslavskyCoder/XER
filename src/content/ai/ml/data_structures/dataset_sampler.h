#pragma once

#include "dataset_base.h"

#include <cstddef>
#include <cstdint>

namespace Engine::ML::DataStructures {

class DatasetSampler {
 public:
	static DatasetBase SampleFraction(const DatasetBase& dataset, float fraction, uint64_t seed);
	static DatasetBase SampleCount(const DatasetBase& dataset, size_t count, uint64_t seed);
};

}  // namespace Engine::ML::DataStructures

