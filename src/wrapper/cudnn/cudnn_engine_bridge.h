#pragma once

#include <string>

namespace engine::bridge::cudnn {

bool IsAvailable();
std::string Summary();

}  // namespace engine::bridge::cudnn