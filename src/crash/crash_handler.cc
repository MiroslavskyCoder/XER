#include "crash_handler.h"

#include "crash_file_writer.h"
#include "crash_report.h"
#include "crash_stack_trace.h"
#include "crash_v8_dump.h"

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <mutex>
#include <string>

// ---------------------------------------------------------------------------
// Global crash context (written only before signals fire)
// ---------------------------------------------------------------------------
namespace {

std::mutex       g_ctx_mutex;
v8::Isolate*     g_isolate    = nullptr;
std::string      g_script_path;
std::string      g_dump_dir;
std::string      g_last_dump_path;

// Tracks whether handlers are currently installed.
std::atomic<bool> g_installed{false};

struct OldHandlers {
    struct sigaction sigsegv;
    struct sigaction sigabrt;
    struct sigaction sigfpe;
    struct sigaction sigbus;
    struct sigaction sigill;
};
OldHandlers g_old_handlers{};

// ---------------------------------------------------------------------------
// Core signal handler  (non-async-safe work is kept out of here; we fork a
// helper or just write straight to a report file via low-level write).
// ---------------------------------------------------------------------------
void HandleSignal(int sig, siginfo_t* info, void* /*ucontext*/) {
    // Guard against recursive signals.
    static volatile sig_atomic_t s_in_handler = 0;
    if (s_in_handler) {
        _exit(sig + 128);
    }
    s_in_handler = 1;

    // Collect what we can.
    CrashReport report;
    report.signal_num  = sig;
    report.signal_name = CrashReport::SignalName(sig);
    report.pid         = static_cast<int>(getpid());
    report.fault_addr  = (info != nullptr) ? info->si_addr : nullptr;
    report.script_path = g_script_path;

    report.native_stack = CrashStackTrace::Capture(/*skip_frames=*/3);

    if (g_isolate != nullptr) {
        report.v8_heap_stats  = CrashV8Dump::CaptureHeapStats(g_isolate);
        report.v8_stack_trace = CrashV8Dump::CaptureCurrentStack(g_isolate);
    }

    const std::string dump_path =
        CrashFileWriter::Write(report, g_dump_dir.empty() ? "." : g_dump_dir);

    // Store for later retrieval (best-effort; may not execute if heap is bad).
    g_last_dump_path = dump_path;

    // Print minimal notice to stderr using the async-signal-safe write().
    {
        const char header[] = "\n[CrashHandler] Segfault/signal – dump written to: ";
        ::write(STDERR_FILENO, header, sizeof(header) - 1);
        ::write(STDERR_FILENO, dump_path.c_str(), dump_path.size());
        ::write(STDERR_FILENO, "\n", 1);
    }

    // Re-raise with the original handler so the OS can produce a core dump.
    const OldHandlers& old = g_old_handlers;
    const struct sigaction* prev = nullptr;
    switch (sig) {
        case SIGSEGV: prev = &old.sigsegv; break;
        case SIGABRT: prev = &old.sigabrt; break;
        case SIGFPE:  prev = &old.sigfpe;  break;
        case SIGBUS:  prev = &old.sigbus;  break;
        case SIGILL:  prev = &old.sigill;  break;
        default: break;
    }
    if (prev && prev->sa_handler != SIG_DFL && prev->sa_handler != SIG_IGN) {
        sigaction(sig, prev, nullptr);
        raise(sig);
    } else {
        signal(sig, SIG_DFL);
        raise(sig);
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void CrashHandler::Install() {
    if (g_installed.exchange(true)) {
        return;  // Already installed.
    }

    struct sigaction sa{};
    sa.sa_sigaction = HandleSignal;
    sa.sa_flags     = SA_SIGINFO | SA_RESETHAND;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGSEGV, &sa, &g_old_handlers.sigsegv);
    sigaction(SIGABRT, &sa, &g_old_handlers.sigabrt);
    sigaction(SIGFPE,  &sa, &g_old_handlers.sigfpe);
    sigaction(SIGBUS,  &sa, &g_old_handlers.sigbus);
    sigaction(SIGILL,  &sa, &g_old_handlers.sigill);
}

void CrashHandler::Uninstall() {
    if (!g_installed.exchange(false)) {
        return;
    }
    sigaction(SIGSEGV, &g_old_handlers.sigsegv, nullptr);
    sigaction(SIGABRT, &g_old_handlers.sigabrt, nullptr);
    sigaction(SIGFPE,  &g_old_handlers.sigfpe,  nullptr);
    sigaction(SIGBUS,  &g_old_handlers.sigbus,  nullptr);
    sigaction(SIGILL,  &g_old_handlers.sigill,  nullptr);
}

void CrashHandler::SetIsolate(v8::Isolate* isolate) {
    std::lock_guard<std::mutex> lk(g_ctx_mutex);
    g_isolate = isolate;
}

void CrashHandler::SetScriptPath(std::string path) {
    std::lock_guard<std::mutex> lk(g_ctx_mutex);
    g_script_path = std::move(path);
}

void CrashHandler::SetDumpDir(std::string dir) {
    std::lock_guard<std::mutex> lk(g_ctx_mutex);
    g_dump_dir = std::move(dir);
}

std::string CrashHandler::LastDumpPath() {
    std::lock_guard<std::mutex> lk(g_ctx_mutex);
    return g_last_dump_path;
}
