#pragma once

#include <string>
#include <unordered_map>

class Provider {
public:
	void set(std::string key, std::string value);

	const std::string* get(const std::string& key) const;

private:
	std::unordered_map<std::string, std::string> values_;
};
