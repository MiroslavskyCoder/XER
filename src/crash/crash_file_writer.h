#pragma once

#include "crash_report.h"

#include <string>

// CrashFileWriter serialises a CrashReport to a timestamped plain-text file.
class CrashFileWriter {
public:
    // Write the report to <dir>/crash_<timestamp>_pid<pid>.txt.
    // Returns the absolute path of the file written, or an empty string on
    // failure.
    static std::string Write(const CrashReport& report, std::string_view dir);

    // Build the filename only (without directory), e.g.
    // "crash_20260423_143012_pid1234.txt"
    static std::string BuildFilename(const CrashReport& report);
};
