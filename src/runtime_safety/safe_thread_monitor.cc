#include "runtime_safety/safe_thread_monitor.h"

#include <string>

#include <absl/strings/str_cat.h>

namespace Engine::RuntimeSafety {

ThreadMonitorSnapshot CaptureThreadMonitorSnapshot(const EngineParams& params,
						   std::size_t resolved_async_workers,
						   std::size_t resolved_queue_depth) {
	ThreadMonitorSnapshot snapshot;
	snapshot.async_io_workers = resolved_async_workers;
	snapshot.async_io_queue_depth = resolved_queue_depth;
	snapshot.max_cpu_threads = params.max_cpu_threads;
	snapshot.v8_platform_workers = params.v8_platform_workers;
	snapshot.cpu_affinity = params.cpu_affinity;
	snapshot.thread_priority = params.thread_priority;
	return snapshot;
}

std::string BuildThreadMonitorSummary(const ThreadMonitorSnapshot& snapshot) {
	return absl::StrCat(
		"thread_monitor(async_workers=", snapshot.async_io_workers,
		", queue_depth=", snapshot.async_io_queue_depth,
		", max_cpu_threads=", snapshot.max_cpu_threads,
		", v8_workers=", snapshot.v8_platform_workers,
		", cpu_affinity=", snapshot.cpu_affinity.empty() ? "auto" : snapshot.cpu_affinity,
		", priority=", snapshot.thread_priority,
		")");
}

}  // namespace Engine::RuntimeSafety
