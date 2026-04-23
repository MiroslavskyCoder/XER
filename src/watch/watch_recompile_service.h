#pragma once

#include <string>

class WatchRecompileService {
public:
    WatchRecompileService();
    ~WatchRecompileService();

    // Rebuild/re-run script with fresh V8 isolate.
    bool Recompile(const std::string& script_path, std::string* error_message) const;

    // Stop currently running watched child process, if any.
    void Stop();

private:
    bool SpawnChild(const std::string& script_path, std::string* error_message);
    bool StopChild(int grace_ms, std::string* error_message);

    mutable int child_pid_ = -1;
};
