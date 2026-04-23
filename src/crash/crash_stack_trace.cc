#include "crash_stack_trace.h"

#include <dlfcn.h>
#include <execinfo.h>

#include <cstdio>
#include <cstring>
#include <sstream>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace {

// Attempt to demangle a C++ symbol using __cxa_demangle if available.
// Falls back to the raw mangled name on failure.
std::string TryDemangle(const char* mangled) {
#if defined(__GNUC__) || defined(__clang__)
    // Dynamic-link to avoid a hard dependency on libstdc++/libc++abi at link time.
    using DemangleFn = char* (*)(const char*, char*, std::size_t*, int*);
    static DemangleFn s_demangle = []() -> DemangleFn {
        void* sym = ::dlsym(RTLD_DEFAULT, "__cxa_demangle");
        return reinterpret_cast<DemangleFn>(sym);
    }();

    if (s_demangle != nullptr) {
        int status = 0;
        char* demangled = s_demangle(mangled, nullptr, nullptr, &status);
        if (status == 0 && demangled != nullptr) {
            std::string result(demangled);
            ::free(demangled);
            return result;
        }
    }
#endif
    return std::string(mangled);
}

// Build a single frame description using dladdr for best symbol resolution.
std::string DescribeFrame(void* addr) {
    Dl_info info{};
    if (::dladdr(addr, &info) != 0) {
        char addr_buf[32];
        std::snprintf(addr_buf, sizeof(addr_buf), "%p", addr);

        // Relative offset within the shared object / executable.
        const std::ptrdiff_t offset =
            static_cast<const char*>(addr) -
            static_cast<const char*>(info.dli_saddr != nullptr ? info.dli_saddr
                                                               : info.dli_fbase);

        std::ostringstream frame;
        frame << addr_buf << "  ";

        if (info.dli_sname != nullptr && info.dli_sname[0] != '\0') {
            frame << TryDemangle(info.dli_sname);
            frame << "+0x" << std::hex << offset;
        } else {
            frame << "(no symbol)";
        }

        if (info.dli_fname != nullptr && info.dli_fname[0] != '\0') {
            // Shorten to just the filename for readability.
            const char* slash = std::strrchr(info.dli_fname, '/');
            frame << "  [" << (slash != nullptr ? slash + 1 : info.dli_fname) << "]";
        }

        return frame.str();
    }

    // dladdr failed – fall back to raw pointer.
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%p  (unresolved)", addr);
    return std::string(buf);
}

}  // namespace

// ---------------------------------------------------------------------------
// CrashStackTrace::Capture
// ---------------------------------------------------------------------------

std::vector<std::string> CrashStackTrace::Capture(int skip_frames) {
    void* raw[kMaxFrames];
    const int count = ::backtrace(raw, kMaxFrames);

    std::vector<std::string> result;
    result.reserve(static_cast<std::size_t>(count));

    const int start = std::min(skip_frames, count);
    for (int i = start; i < count; ++i) {
        result.push_back(DescribeFrame(raw[i]));
    }
    return result;
}

// ---------------------------------------------------------------------------
// CrashStackTrace::Format
// ---------------------------------------------------------------------------

std::string CrashStackTrace::Format(const std::vector<std::string>& frames) {
    std::ostringstream out;
    for (std::size_t i = 0; i < frames.size(); ++i) {
        out << "  #" << i << "  " << frames[i] << "\n";
    }
    return out.str();
}
