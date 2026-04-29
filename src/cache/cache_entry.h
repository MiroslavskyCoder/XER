#pragma once

#include <string>

#include <absl/strings/string_view.h>

namespace Engine::Cache {

struct CacheEntry {
	std::string scope;
	std::string key;
	std::string name;
};

CacheEntry MakeEntry(absl::string_view scope,
		     absl::string_view key,
		     absl::string_view name);
bool IsValid(const CacheEntry& entry);

}  // namespace Engine::Cache
