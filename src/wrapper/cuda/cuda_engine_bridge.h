#pragma once

#include <string>

namespace engine::bridge::cuda {

bool IsAvailable();
std::string Summary();

}  // namespace engine::bridge::cuda