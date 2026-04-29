#include "provider.h"

#include <algorithm>
#include <utility>

std::vector<std::string> Provider::keys() const {
	std::vector<std::string> out;
	out.reserve(values_.size());
	for (const auto& entry : values_) {
		out.push_back(entry.first);
	}
	std::sort(out.begin(), out.end());
	return out;
}

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
