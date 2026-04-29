#include "cache/cache_entry.h"

#include <array>

#include <range/v3/algorithm/all_of.hpp>

#include "helper/string.h"

namespace Engine::Cache {

CacheEntry MakeEntry(absl::string_view scope,
		     absl::string_view key,
		     absl::string_view name) {
	return CacheEntry{
		Helper::String::CanonicalizeToken(scope),
		Helper::String::NormalizeUtf8(key),
		Helper::String::NormalizeUtf8(name),
	};
}

bool IsValid(const CacheEntry& entry) {
	return ranges::all_of(std::array<std::string, 3>{entry.scope, entry.key, entry.name},
		[](const std::string& part) {
			return !Helper::String::IsBlank(part);
		});
}

}  // namespace Engine::Cache
