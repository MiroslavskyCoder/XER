#include "crash_v8_dump.h"

#include <iomanip>
#include <sstream>

// ---------------------------------------------------------------------------
// Helper: V8 string → std::string (returns "(empty)" on failure)
// ---------------------------------------------------------------------------
namespace {

std::string V8Str(v8::Isolate* isolate, v8::Local<v8::Value> value) {
    if (value.IsEmpty() || !value->IsString()) {
        return std::string();
    }
    v8::String::Utf8Value utf8(isolate, value);
    if (*utf8 == nullptr) {
        return std::string();
    }
    return std::string(*utf8, static_cast<std::size_t>(utf8.length()));
}

}  // namespace

// ---------------------------------------------------------------------------
// CaptureHeapStats
// ---------------------------------------------------------------------------

std::string CrashV8Dump::CaptureHeapStats(v8::Isolate* isolate) {
    if (isolate == nullptr) {
        return std::string();
    }

    v8::HeapStatistics hs;
    isolate->GetHeapStatistics(&hs);

    auto mib = [](std::size_t bytes) -> double {
        return static_cast<double>(bytes) / (1024.0 * 1024.0);
    };

    std::ostringstream out;
    out << std::fixed << std::setprecision(2);
    out << "  total_heap_size             : " << mib(hs.total_heap_size())             << " MiB\n";
    out << "  total_heap_size_executable  : " << mib(hs.total_heap_size_executable())  << " MiB\n";
    out << "  total_physical_size         : " << mib(hs.total_physical_size())         << " MiB\n";
    out << "  total_available_size        : " << mib(hs.total_available_size())        << " MiB\n";
    out << "  used_heap_size              : " << mib(hs.used_heap_size())              << " MiB\n";
    out << "  heap_size_limit             : " << mib(hs.heap_size_limit())             << " MiB\n";
    out << "  malloced_memory             : " << mib(hs.malloced_memory())             << " MiB\n";
    out << "  peak_malloced_memory        : " << mib(hs.peak_malloced_memory())        << " MiB\n";
    out << "  number_of_native_contexts   : " << hs.number_of_native_contexts()        << "\n";
    out << "  number_of_detached_contexts : " << hs.number_of_detached_contexts()      << "\n";
    out << "  does_zap_garbage            : " << hs.does_zap_garbage()                 << "\n";

    return out.str();
}

// ---------------------------------------------------------------------------
// CaptureCurrentStack
// ---------------------------------------------------------------------------

std::string CrashV8Dump::CaptureCurrentStack(v8::Isolate* isolate) {
    if (isolate == nullptr) {
        return std::string();
    }

    // StackTrace::CurrentStackTrace must be called on the isolate's thread.
    // During a SIGSEGV this is usually the same thread, but the JS state may
    // be partially corrupt; we guard with a TryCatch.
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::StackTrace> stack_trace =
        v8::StackTrace::CurrentStackTrace(isolate, kMaxFrames,
                                          v8::StackTrace::kDetailed);

    if (stack_trace.IsEmpty()) {
        return std::string();
    }

    const int frame_count = stack_trace->GetFrameCount();
    if (frame_count == 0) {
        return std::string();
    }

    std::ostringstream out;
    for (int i = 0; i < frame_count; ++i) {
        v8::Local<v8::StackFrame> frame = stack_trace->GetFrame(isolate, i);
        if (frame.IsEmpty()) {
            continue;
        }

        const std::string func   = V8Str(isolate, frame->GetFunctionName());
        const std::string script = V8Str(isolate, frame->GetScriptName());
        const int line   = frame->GetLineNumber();
        const int column = frame->GetColumn();

        out << "  at ";
        if (!func.empty()) {
            out << func << " ";
        }
        out << "(";
        if (!script.empty()) {
            out << script << ":" << line << ":" << column;
        } else {
            out << "<anonymous>:" << line << ":" << column;
        }
        out << ")\n";
    }
    return out.str();
}

// ---------------------------------------------------------------------------
// CaptureContextInfo
// ---------------------------------------------------------------------------

std::string CrashV8Dump::CaptureContextInfo(v8::Isolate* isolate) {
    if (isolate == nullptr) {
        return std::string();
    }

    v8::HeapStatistics hs;
    isolate->GetHeapStatistics(&hs);

    std::ostringstream out;
    out << "  native_contexts  : " << hs.number_of_native_contexts() << "\n";
    out << "  detached_contexts: " << hs.number_of_detached_contexts() << "\n";
    return out.str();
}
