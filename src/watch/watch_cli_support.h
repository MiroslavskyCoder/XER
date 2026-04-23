#pragma once

#include <string>

struct WatchCliSupport {
    static std::string Header(const std::string& script_path, int poll_interval_ms);
    static std::string CycleMessage(int cycle, const std::string& reason);
};
