/**
 * @file logger.cc
 * @brief Logger implementation for EngineDoctor.
 */

#include "core/logger.h"

#include <atomic>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <sstream>

#include "flux/terminal/terminal_output_renderer.h"

namespace EngineDoctor {
namespace {

std::atomic<int> g_log_level{1};
std::mutex g_log_mutex;

const char* LevelName(Logger::Level level) {
	switch (level) {
	case Logger::Level::Debug:
		return "DEBUG";
	case Logger::Level::Info:
		return "INFO";
	case Logger::Level::Warning:
		return "WARN";
	case Logger::Level::Error:
		return "ERROR";
	}
	return "INFO";
}

std::string BuildTimestamp() {
	const std::time_t now = std::time(nullptr);
	std::tm tm_now {};
#if defined(_WIN32)
	localtime_s(&tm_now, &now);
#else
	localtime_r(&now, &tm_now);
#endif
	std::ostringstream output;
	output << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");
	return output.str();
}

} // namespace

void Logger::initialize(int log_level) {
	g_log_level.store(log_level < 0 ? 0 : log_level);
}

void Logger::debug(const std::string& message) {
	log(Level::Debug, message);
}

void Logger::info(const std::string& message) {
	log(Level::Info, message);
}

void Logger::warning(const std::string& message) {
	log(Level::Warning, message);
}

void Logger::error(const std::string& message) {
	log(Level::Error, message);
}

void Logger::log(Level level, const std::string& message) {
	if (static_cast<int>(level) < g_log_level.load()) {
		return;
	}

	std::lock_guard<std::mutex> lock(g_log_mutex);
	std::ostringstream output;
	output << "[doctor][" << LevelName(level) << "][" << BuildTimestamp() << "] "
		   << message;
	flux::terminal::WriteLine(
		level == Level::Error ? flux::terminal::OutputStream::kStderr
		                      : flux::terminal::OutputStream::kStdout,
		output.str());
}

} // namespace EngineDoctor
