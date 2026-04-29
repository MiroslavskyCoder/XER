/**
 * @file module_manager.cc
 * @brief ModuleManager implementation.
 */

#include "core/module_manager.h"

namespace EngineDoctor {

ModuleManager::ModuleManager(Context& context)
	: context_(context) {}

void ModuleManager::initialize_all(const Config& config) {
	for (const auto& initializer : initializers_) {
		initializer(config);
	}
}

} // namespace EngineDoctor
