#pragma once

#include <atomic>
#include <string>

struct WatchLiveUpdatexScriptConfig {
    std::string script_path;
    int poll_interval_ms = 350;
    int max_cycles = -1;  // -1 = infinite
};

class WatchLiveUpdatexScript {
public:
    explicit WatchLiveUpdatexScript(WatchLiveUpdatexScriptConfig config);

    // Run watch loop. Returns 0 when the loop exits cleanly.
    int Run();

    static void RequestStop();
    static bool StopRequested();

private:
    static std::atomic<bool> stop_requested_;
    WatchLiveUpdatexScriptConfig config_;
};
