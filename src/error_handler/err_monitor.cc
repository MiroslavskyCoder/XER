#include "error_handler/err_monitor.h"

#include <cstdlib>
#include <ctime>
#include <exception>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

#include <absl/strings/str_cat.h>
#include <range/v3/algorithm/any_of.hpp>

#include "flux/terminal/terminal_output_renderer.h"
#include "helper/string.h"

namespace Engine::ErrorHandler {
namespace {

std::mutex& MonitorMutex() {
	static std::mutex mutex;
	return mutex;
}

MonitorConfiguration& MutableConfiguration() {
	static MonitorConfiguration configuration;
	return configuration;
}

bool& MonitorInstalled() {
	static bool installed = false;
	return installed;
}

std::string TimestampPrefix() {
	if (!MutableConfiguration().timestamps) {
		return std::string();
	}

	const std::time_t now = std::time(nullptr);
	std::tm local_time{};
	localtime_r(&now, &local_time);
	std::ostringstream out;
	out << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S") << " ";
	return out.str();
}

std::string FormatLine(std::string_view level,
					 std::string_view component,
					 std::string_view message) {
	const absl::string_view level_view(level.data(), level.size());
	const absl::string_view component_view(component.data(), component.size());
	const absl::string_view message_view(message.data(), message.size());
	return absl::StrCat(
		TimestampPrefix(),
		"[", level_view, "] ",
		Helper::String::DefaultString(component_view, "startup"),
		": ",
		Helper::String::NormalizeUtf8(message_view));
}

bool ShouldEmitInfo() {
	const MonitorConfiguration& config = MutableConfiguration();
	if (config.verbose) {
		return true;
	}
	const std::string level = Helper::String::CanonicalizeToken(config.log_level);
	return ranges::any_of(std::initializer_list<std::string_view>{"info", "debug", "trace"},
		[&level](std::string_view candidate) {
			return level == candidate;
		});
}

[[noreturn]] void HandleTerminate() {
	std::string detail = "std::terminate invoked";
	if (std::exception_ptr current = std::current_exception(); current != nullptr) {
		try {
			std::rethrow_exception(current);
		} catch (const std::exception& error) {
			detail = absl::StrCat(detail, ": ", error.what());
		} catch (...) {
			detail = absl::StrCat(detail, ": unknown exception");
		}
	}
	flux::terminal::WriteLine(flux::terminal::OutputStream::kStderr,
		FormatLine("fatal", "error_handler", detail));
	std::abort();
}

}  // namespace

void InitializeMonitor(const MonitorConfiguration& config) {
	bool should_emit_initialized = false;
	{
		std::lock_guard<std::mutex> lock(MonitorMutex());
		MutableConfiguration() = config;
		if (!MonitorInstalled()) {
			std::set_terminate(&HandleTerminate);
			MonitorInstalled() = true;
		}
		should_emit_initialized = ShouldEmitInfo();
	}
	if (should_emit_initialized) {
		ReportStartupEvent("error_handler", "monitor initialized");
	}
}

void ReportStartupEvent(std::string_view component, std::string_view message) {
	if (!ShouldEmitInfo()) {
		return;
	}
	flux::terminal::WriteLine(flux::terminal::OutputStream::kStderr,
		FormatLine("info", component, message));
}

void ReportStartupError(std::string_view component, std::string_view message) {
	flux::terminal::WriteLine(flux::terminal::OutputStream::kStderr,
		FormatLine("error", component, message));
}

}  // namespace Engine::ErrorHandler
