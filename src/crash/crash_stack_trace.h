#pragma once

#include <string>
#include <vector>

// CrashStackTrace captures the native (C++) call stack of the current thread
// using POSIX backtrace()/backtrace_symbols() and, when available, resolves
// symbol names via dladdr().
class CrashStackTrace {
public:
    // Capture at most kMaxFrames frames.  skip_frames skips the topmost
    // frames (useful to omit the crash handler itself from the trace).
    // Returns one human-readable string per frame.
    static std::vector<std::string> Capture(int skip_frames = 0);

    // Render the captured frames as a single indented string block.
    static std::string Format(const std::vector<std::string>& frames);

private:
    static constexpr int kMaxFrames = 128;
};
