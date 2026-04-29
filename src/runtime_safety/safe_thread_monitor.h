#pragma once

#include <cstddef>
#include <string>

#include "engine_params.h"

namespace Engine::RuntimeSafety {

struct ThreadMonitorSnapshot {
	std::size_t async_io_workers = 0;
	std::size_t async_io_queue_depth = 0;
	int max_cpu_threads = -1;
	int v8_platform_workers = -1;
	std::string cpu_affinity;
	int thread_priority = 0;
};

ThreadMonitorSnapshot CaptureThreadMonitorSnapshot(const EngineParams& params,
						   std::size_t resolved_async_workers,
						   std::size_t resolved_queue_depth);
std::string BuildThreadMonitorSummary(const ThreadMonitorSnapshot& snapshot);

}  // namespace Engine::RuntimeSafety
