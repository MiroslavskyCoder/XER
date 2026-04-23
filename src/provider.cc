#include "provider.h"

#include <utility>

void Provider::set(std::string key, std::string value) {
	values_[std::move(key)] = std::move(value);
}

const std::string* Provider::get(const std::string& key) const {
	auto it = values_.find(key);
	if (it == values_.end()) {
		return nullptr;
	}

	return &it->second;
}
