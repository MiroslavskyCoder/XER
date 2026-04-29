/**
 * @file module_manager.h
 * @brief Registration and initialization of EngineDoctor modules.
 */

#pragma once

#include "core/engine_doctor_config.h"
#include "core/engine_doctor_context.h"

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace EngineDoctor {

namespace detail {

template <typename Module, typename = void>
struct HasInitialize : std::false_type {};

template <typename Module>
struct HasInitialize<
	Module,
	std::void_t<decltype(std::declval<Module&>().initialize(std::declval<const Config&>()))>>
	: std::true_type {};

template <typename Module>
void InitializeIfAvailable(Module& module, const Config& config) {
	if constexpr (HasInitialize<Module>::value) {
		module.initialize(config);
	}
}

} // namespace detail

class ModuleManager {
public:
	explicit ModuleManager(Context& context);

	template <typename T>
	T* register_module(std::unique_ptr<T> module) {
		auto shared_module = std::shared_ptr<T>(std::move(module));
		T* raw_module = shared_module.get();
		context_.set_module<T>(shared_module);
		initializers_.push_back([shared_module](const Config& config) {
			detail::InitializeIfAvailable(*shared_module, config);
		});
		return raw_module;
	}

	void initialize_all(const Config& config);

private:
	Context& context_;
	std::vector<std::function<void(const Config&)>> initializers_;
};

} // namespace EngineDoctor
