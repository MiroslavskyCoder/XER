#pragma once

#include <unordered_map>

namespace absl {

template <typename Key, typename Value, typename Hash = std::hash<Key>, typename Eq = std::equal_to<Key>, typename Alloc = std::allocator<std::pair<const Key, Value>>>
using flat_hash_map = std::unordered_map<Key, Value, Hash, Eq, Alloc>;

}  // namespace absl