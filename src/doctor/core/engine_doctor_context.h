/**
 * @file engine_doctor_context.h
 * @brief Shared module and data registry for EngineDoctor.
 */

#pragma once

#include "core/event_bus.h"

#include <any>
#include <memory>
#include <mutex>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace EngineDoctor {

class Context {
public:
	Context() = default;
	Context(const Context&) = delete;
	Context& operator=(const Context&) = delete;

	template <typename T>
	void set_module(std::shared_ptr<T> module) {
		std::lock_guard<std::mutex> lock(mutex_);
		modules_[std::type_index(typeid(T))] = std::move(module);
	}

	template <typename T>
	T* get_module() const {
		std::lock_guard<std::mutex> lock(mutex_);
		const auto it = modules_.find(std::type_index(typeid(T)));
		if (it == modules_.end()) {
			return nullptr;
		}
		return static_cast<T*>(it->second.get());
	}

	template <typename T>
	void set_data(const std::string& key, T value) {
		std::lock_guard<std::mutex> lock(mutex_);
		data_[key] = std::move(value);
	}

	template <typename T>
	T* get_data(const std::string& key) {
		std::lock_guard<std::mutex> lock(mutex_);
		const auto it = data_.find(key);
		if (it == data_.end()) {
			return nullptr;
		}
		return std::any_cast<T>(&it->second);
	}

	template <typename T>
	const T* get_data(const std::string& key) const {
		std::lock_guard<std::mutex> lock(mutex_);
		const auto it = data_.find(key);
		if (it == data_.end()) {
			return nullptr;
		}
		return std::any_cast<T>(&it->second);
	}

	bool has_data(const std::string& key) const;
	EventBus& event_bus();
	const EventBus& event_bus() const;

private:
	mutable std::mutex mutex_;
	std::unordered_map<std::type_index, std::shared_ptr<void>> modules_;
	std::unordered_map<std::string, std::any> data_;
	EventBus event_bus_;
};

} // namespace EngineDoctor
