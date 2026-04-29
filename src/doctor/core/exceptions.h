/**
 * @file exceptions.h
 * @brief Common exceptions used by EngineDoctor.
 */

#pragma once

#include <stdexcept>
#include <string>

namespace EngineDoctor {

class DoctorError : public std::runtime_error {
public:
	explicit DoctorError(const std::string& message)
		: std::runtime_error(message) {}
};

class ModuleNotFoundError : public DoctorError {
public:
	explicit ModuleNotFoundError(const std::string& module_name)
		: DoctorError("EngineDoctor module not found: " + module_name) {}
};

class InvalidConfigurationError : public DoctorError {
public:
	explicit InvalidConfigurationError(const std::string& message)
		: DoctorError("Invalid EngineDoctor configuration: " + message) {}
};

class DataAccessError : public DoctorError {
public:
	explicit DataAccessError(const std::string& message)
		: DoctorError("EngineDoctor data access error: " + message) {}
};

} // namespace EngineDoctor
