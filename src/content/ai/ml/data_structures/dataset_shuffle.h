#pragma once

#include "dataset_base.h"

#include <cstdint>

namespace Engine::ML::DataStructures {

class DatasetShuffle {
 public:
	static DatasetBase ShuffledCopy(const DatasetBase& dataset, uint64_t seed);
	static void ShuffleInPlace(DatasetBase& dataset, uint64_t seed);
};

}  // namespace Engine::ML::DataStructures

