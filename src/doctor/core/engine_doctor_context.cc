/**
 * @file engine_doctor_context.cc
 * @brief Context implementation for EngineDoctor.
 */

#include "core/engine_doctor_context.h"

namespace EngineDoctor {

bool Context::has_data(const std::string& key) const {
	std::lock_guard<std::mutex> lock(mutex_);
	return data_.find(key) != data_.end();
}

} // namespace EngineDoctor
