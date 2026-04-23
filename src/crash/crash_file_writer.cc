#include "crash_file_writer.h"

#include <fcntl.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace {

std::string TimestampTag() {
    const auto tp   = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&t), "%Y%m%d_%H%M%S");
    return ss.str();
}

// Write a string to a file descriptor using raw write() (async-signal-safe).
bool WriteAll(int fd, std::string_view data) {
    const char* p   = data.data();
    std::size_t rem = data.size();
    while (rem > 0) {
        const ssize_t n = ::write(fd, p, rem);
        if (n <= 0) {
            return false;
        }
        p   += n;
        rem -= static_cast<std::size_t>(n);
    }
    return true;
}

}  // namespace

// ---------------------------------------------------------------------------
// CrashFileWriter::BuildFilename
// ---------------------------------------------------------------------------

std::string CrashFileWriter::BuildFilename(const CrashReport& report) {
    std::ostringstream ss;
    ss << "crash_" << TimestampTag() << "_pid" << report.pid << ".txt";
    return ss.str();
}

// ---------------------------------------------------------------------------
// CrashFileWriter::Write
// ---------------------------------------------------------------------------

std::string CrashFileWriter::Write(const CrashReport& report,
                                    std::string_view dir) {
    // Build output path.
    std::filesystem::path out_dir(dir.empty() ? "." : std::string(dir));

    // Ensure the directory exists (best-effort; ignore errors in signal ctx).
    std::error_code ec;
    std::filesystem::create_directories(out_dir, ec);

    const std::filesystem::path out_path = out_dir / BuildFilename(report);
    const std::string path_str = out_path.string();

    // Serialise the report to text.
    const std::string text = report.FormatText();

    // Open the file using the low-level open() which is async-signal-safe.
    const int fd = ::open(path_str.c_str(),
                          O_WRONLY | O_CREAT | O_TRUNC,
                          0644);
    if (fd < 0) {
        // Try current directory as a fallback.
        const std::string fallback = "./" + BuildFilename(report);
        const int fd2 = ::open(fallback.c_str(),
                               O_WRONLY | O_CREAT | O_TRUNC,
                               0644);
        if (fd2 < 0) {
            return std::string();
        }
        WriteAll(fd2, text);
        ::close(fd2);
        return fallback;
    }

    WriteAll(fd, text);
    ::close(fd);
    return path_str;
}
