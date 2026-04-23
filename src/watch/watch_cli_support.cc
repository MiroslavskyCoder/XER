#include "watch/watch_cli_support.h"

#include <sstream>

std::string WatchCliSupport::Header(const std::string& script_path, int poll_interval_ms) {
    std::ostringstream out;
    out << "[watch] WatchLiveUpdatexScript started\n";
    out << "[watch] script: " << script_path << "\n";
    out << "[watch] poll interval ms: " << poll_interval_ms << "\n";
    out << "[watch] press Ctrl+C to stop\n";
    return out.str();
}

std::string WatchCliSupport::CycleMessage(int cycle, const std::string& reason) {
    std::ostringstream out;
    out << "[watch] cycle=" << cycle << " trigger=" << reason;
    return out.str();
}
