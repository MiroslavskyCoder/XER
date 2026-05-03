#pragma once

#include <string>

namespace Engine::ML::Utils {

class MlVersionManager {
 public:
	static std::string GetVersion();
	static std::string GetBuildTag();
	static std::string GetBackendSummary();
};

}  // namespace Engine::ML::Utils

