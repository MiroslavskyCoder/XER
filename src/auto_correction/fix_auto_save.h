#pragma once

#include <string>
#include <thread>
#include <atomic>

namespace AutoCorrection {

class FixAutoSave {
public:
    FixAutoSave() = default;
    ~FixAutoSave();

    void Start(const std::string& path, int interval_ms);
    void Stop();
    void Flush();
    bool IsRunning() const;

private:
    void Loop();

    std::string path_;
    int interval_ms_ = 1000;
    std::atomic<bool> running_{false};
    std::thread thread_;
};

} // namespace AutoCorrection
