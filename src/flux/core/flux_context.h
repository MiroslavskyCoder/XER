#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "flux/core/flux_config.h"
#include "flux/core/logger.h"
#include "flux/terminal/terminal_window.h"

namespace flux::core {

class FluxContext {
public:
	explicit FluxContext(FluxConfig config = {});

	FluxConfig& config();
	const FluxConfig& config() const;

	Logger& logger();
	const Logger& logger() const;

	flux::terminal::TerminalWindow& window();
	const flux::terminal::TerminalWindow& window() const;

	void RefreshTerminalWindow();

	void SetValue(std::string key, std::string value);
	bool HasValue(std::string_view key) const;
	std::string GetValue(std::string_view key) const;
	std::vector<std::string> Keys() const;

private:
	FluxConfig config_;
	Logger logger_;
	flux::terminal::TerminalWindow window_;
	std::unordered_map<std::string, std::string> values_;
};

}  // namespace flux::core
