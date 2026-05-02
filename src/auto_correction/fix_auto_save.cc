#include "fix_auto_save.h"

#include <fstream>
#include <chrono>

namespace AutoCorrection {

FixAutoSave::~FixAutoSave() {
    Stop();
}

void FixAutoSave::Start(const std::string& path, int interval_ms) {
    if (running_.load()) {
        Stop();
    }
    path_ = path;
    interval_ms_ = interval_ms;
    running_.store(true);
    thread_ = std::thread(&FixAutoSave::Loop, this);
}

void FixAutoSave::Stop() {
    running_.store(false);
    if (thread_.joinable()) {
        thread_.join();
    }
}

void FixAutoSave::Flush() {
    if (path_.empty()) return;
    // Force a write cycle: touch the file to ensure OS buffers are flushed.
    std::ofstream ofs(path_, std::ios::app);
    if (ofs.is_open()) {
        ofs.flush();
    }
}

bool FixAutoSave::IsRunning() const {
    return running_.load();
}

void FixAutoSave::Loop() {
    while (running_.load()) {
        Flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms_));
    }
}

} // namespace AutoCorrection
