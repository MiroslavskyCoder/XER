#include "feature.h"

#include <utility>

Feature::Feature(std::string name) : name_(std::move(name)) {}

const std::string& Feature::name() const {
	return name_;
}
