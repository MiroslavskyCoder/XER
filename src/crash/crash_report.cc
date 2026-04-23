#include "crash_report.h"

#include <signal.h>

#include <chrono>
#include <cstdio>
#include <iomanip>
#include <sstream>

// ---------------------------------------------------------------------------
// SignalName
// ---------------------------------------------------------------------------

const char* CrashReport::SignalName(int sig) {
    switch (sig) {
        case SIGSEGV: return "SIGSEGV (Segmentation fault)";
        case SIGABRT: return "SIGABRT (Abort)";
        case SIGFPE:  return "SIGFPE  (Floating-point exception)";
        case SIGBUS:  return "SIGBUS  (Bus error)";
        case SIGILL:  return "SIGILL  (Illegal instruction)";
        default:      return "UNKNOWN";
    }
}

// ---------------------------------------------------------------------------
// FormatText helpers
// ---------------------------------------------------------------------------

namespace {

std::string Now() {
    const auto tp  = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void Section(std::ostringstream& out, std::string_view title) {
    out << "\n";
    out << "=================================================================\n";
    out << "  " << title << "\n";
    out << "=================================================================\n";
}

}  // namespace

// ---------------------------------------------------------------------------
// FormatText
// ---------------------------------------------------------------------------

std::string CrashReport::FormatText() const {
    std::ostringstream out;

    out << "EngineBuilder Crash Report\n";
    out << "Generated  : " << Now() << "\n";
    out << "PID        : " << pid << "\n";

    // -----------------------------------------------------------------------
    Section(out, "Signal");
    // -----------------------------------------------------------------------
    out << "Signal     : " << signal_name << " (" << signal_num << ")\n";
    if (fault_addr != nullptr) {
        char addr_buf[32];
        std::snprintf(addr_buf, sizeof(addr_buf), "%p", fault_addr);
        out << "Fault addr : " << addr_buf << "\n";
    }

    // -----------------------------------------------------------------------
    Section(out, "Script");
    // -----------------------------------------------------------------------
    if (!script_path.empty()) {
        out << script_path << "\n";
    } else {
        out << "(no script path recorded)\n";
    }

    // -----------------------------------------------------------------------
    Section(out, "Native Stack Trace");
    // -----------------------------------------------------------------------
    if (native_stack.empty()) {
        out << "(unavailable)\n";
    } else {
        for (std::size_t i = 0; i < native_stack.size(); ++i) {
            out << "  #" << std::setw(3) << i << "  " << native_stack[i] << "\n";
        }
    }

    // -----------------------------------------------------------------------
    Section(out, "V8 JavaScript Stack Trace");
    // -----------------------------------------------------------------------
    if (v8_stack_trace.empty()) {
        out << "(unavailable or V8 isolate was not registered)\n";
    } else {
        out << v8_stack_trace << "\n";
    }

    // -----------------------------------------------------------------------
    Section(out, "V8 Heap Statistics");
    // -----------------------------------------------------------------------
    if (v8_heap_stats.empty()) {
        out << "(unavailable)\n";
    } else {
        out << v8_heap_stats << "\n";
    }

    out << "\n-- end of crash report --\n";
    return out.str();
}
