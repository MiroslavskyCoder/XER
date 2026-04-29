#include "runtime_safety/safe_integrity_check.h"

#include <algorithm>
#include <string>
#include <vector>

#include <absl/strings/str_cat.h>
#include <absl/strings/str_join.h>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/filter.hpp>

#include "async_io/io_thread_pool.h"
#include "cache/cache_configuration.h"
#include "error_handler/err_monitor.h"
#include "helper/string.h"

namespace Engine::RuntimeSafety {
namespace {

std::size_t ResolveQueueDepth(const StartupOptions& options, std::size_t worker_count) {
	if (options.async_io_queue_depth > 0) {
		return static_cast<std::size_t>(options.async_io_queue_depth);
	}
	return std::max<std::size_t>(8, worker_count * 8);
}

std::string BuildStartupSummary(const StartupOptions& options, const StartupState& state) {
	std::vector<std::string> lines = {
		Helper::String::BuildKeyValueLine("cache_dir", state.cache_directory.string()),
		Helper::String::BuildKeyValueLine("async_io_workers", std::to_string(state.async_io_workers)),
		Helper::String::BuildKeyValueLine("async_io_queue_depth", std::to_string(state.async_io_queue_depth)),
		Helper::String::BuildKeyValueLine("sandbox", state.sandbox_enabled ? "enabled" : "disabled"),
		options.script_path.empty() ? std::string() : Helper::String::BuildKeyValueLine("script", options.script_path.string())
	};
	const auto filtered = lines
		| ranges::views::filter([](const std::string& line) {
			return !Helper::String::IsBlank(line);
		})
		| ranges::to<std::vector<std::string>>();
	return absl::StrJoin(filtered, ", ");
}

}  // namespace

bool BootstrapStartup(const StartupOptions& options, StartupState* state, std::string* error_out) {
	if (state == nullptr) {
		if (error_out != nullptr) {
			*error_out = "Startup state target is null";
		}
		return false;
	}

	const Engine::Cache::CacheConfiguration cache_config = Engine::Cache::CacheConfigurationFromEnvironment();
	if (!Engine::Cache::PrepareCacheDirectory(cache_config, error_out)) {
		Engine::ErrorHandler::ReportStartupError("runtime_safety",
			error_out != nullptr ? *error_out : std::string("Failed to prepare cache directory"));
		return false;
	}

	IO::AsyncIO::IOThreadPool& thread_pool = IO::AsyncIO::IOThreadPool::GetSharedInstance();

	StartupState resolved_state;
	resolved_state.cache_directory = cache_config.directory;
	resolved_state.async_io_workers = thread_pool.GetThreadCount();
	resolved_state.async_io_queue_depth = ResolveQueueDepth(options, resolved_state.async_io_workers);
	resolved_state.sandbox_enabled = options.sandbox;
	*state = resolved_state;

	if (options.verbose) {
		Engine::ErrorHandler::ReportStartupEvent("runtime_safety", BuildStartupSummary(options, resolved_state));
	}
	return true;
}

}  // namespace Engine::RuntimeSafety
