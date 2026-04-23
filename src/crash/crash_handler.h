#pragma once

#include <v8.h>

#include <string>

// CrashHandler installs POSIX signal handlers (SIGSEGV, SIGABRT, SIGFPE,
// SIGBUS, SIGILL) that produce a structured dump report before the process
// terminates.  Call Install() once during startup.  Supply context via the
// Set* helpers so the report contains V8 and script information.
class CrashHandler {
public:
    // Install all signal handlers.  Safe to call multiple times (idempotent).
    static void Install();

    // Uninstall all signal handlers and restore previous handlers.
    static void Uninstall();

    // Optional: provide the active V8 Isolate for heap / JS stack reporting.
    static void SetIsolate(v8::Isolate* isolate);

    // Optional: provide the path of the script being executed.
    static void SetScriptPath(std::string path);

    // Optional: directory where crash dump files are written.
    // Defaults to the current working directory when not set.
    static void SetDumpDir(std::string dir);

    // Returns the path written by the most recent crash report, or empty if
    // no report has been written yet.
    static std::string LastDumpPath();
};
