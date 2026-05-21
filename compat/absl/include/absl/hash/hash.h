#pragma once

#include <functional>

namespace absl {

template <typename T>
using Hash = std::hash<T>;

}  // namespace absl