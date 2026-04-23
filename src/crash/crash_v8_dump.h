#pragma once

#include <v8.h>

#include <string>

// CrashV8Dump collects V8-level diagnostic information from an Isolate.
// All functions are best-effort: they return empty strings when the isolate
// state is too corrupt to query safely.
class CrashV8Dump {
public:
    // Capture a formatted summary of V8 heap statistics (sizes, counts).
    static std::string CaptureHeapStats(v8::Isolate* isolate);

    // Capture the current V8 JavaScript call-stack as a plain text string.
    // Returns up to kMaxFrames frames.  Returns an empty string when no
    // current JS frames are available.
    static std::string CaptureCurrentStack(v8::Isolate* isolate);

    // Capture a list of all live V8 contexts (count + their global
    // object type names where obtainable).
    static std::string CaptureContextInfo(v8::Isolate* isolate);

private:
    static constexpr int kMaxFrames = 64;
};
