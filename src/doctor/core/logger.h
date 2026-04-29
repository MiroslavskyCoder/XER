/**
 * @file logger.h
 * @brief Lightweight thread-safe logger for EngineDoctor.
 */

#pragma once

#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace EngineDoctor {

class Logger {
public:
	enum class Level {
		Debug = 0,
		Info = 1,
		Warning = 2,
		Error = 3,
	};

	static void initialize(int log_level);

	static void debug(const std::string& message);
	static void info(const std::string& message);
	static void warning(const std::string& message);
	static void error(const std::string& message);

	template <typename... Args>
	static void debug(const char* format_string, Args&&... args) {
		log_formatted(Level::Debug, format_string, std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void info(const char* format_string, Args&&... args) {
		log_formatted(Level::Info, format_string, std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void warning(const char* format_string, Args&&... args) {
		log_formatted(Level::Warning, format_string, std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void error(const char* format_string, Args&&... args) {
		log_formatted(Level::Error, format_string, std::forward<Args>(args)...);
	}

private:
	static void log(Level level, const std::string& message);

	static std::string format(const char* format_string) {
		return format_string == nullptr ? std::string() : std::string(format_string);
	}

	template <typename... Args>
	static std::string format(const char* format_string, Args&&... args) {
		const int size = std::snprintf(nullptr, 0, format_string, std::forward<Args>(args)...);
		if (size <= 0) {
			return format(format_string);
		}
		std::vector<char> buffer(static_cast<size_t>(size) + 1u, '\0');
		std::snprintf(buffer.data(), buffer.size(), format_string, std::forward<Args>(args)...);
		return std::string(buffer.data(), static_cast<size_t>(size));
	}

	template <typename... Args>
	static void log_formatted(Level level, const char* format_string, Args&&... args) {
		log(level, format(format_string, std::forward<Args>(args)...));
	}
};

} // namespace EngineDoctor
