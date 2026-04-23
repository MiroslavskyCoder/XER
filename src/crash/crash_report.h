#pragma once

#include <string>
#include <vector>

// CrashReport is a plain-data aggregate that holds all information gathered
// at the moment a fatal signal fires.  It is populated by CrashHandler and
// serialised to text by CrashFileWriter.
struct CrashReport {
    // -----------------------------------------------------------------------
    // Signal information
    // -----------------------------------------------------------------------
    int         signal_num  = 0;
    std::string signal_name;
    void*       fault_addr  = nullptr;

    // -----------------------------------------------------------------------
    // Process information
    // -----------------------------------------------------------------------
    int         pid         = 0;
    std::string script_path;

    // -----------------------------------------------------------------------
    // Native stack trace (one frame per entry)
    // -----------------------------------------------------------------------
    std::vector<std::string> native_stack;

    // -----------------------------------------------------------------------
    // V8 information (may be empty when the isolate is unavailable)
    // -----------------------------------------------------------------------
    std::string v8_heap_stats;
    std::string v8_stack_trace;

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    // Returns a human-readable signal name for common signals.
    static const char* SignalName(int sig);

    // Render the report as a human-readable multi-section text.
    std::string FormatText() const;
};
