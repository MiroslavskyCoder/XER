#include "watch/watch_file_snapshot.h"

#include <system_error>

WatchFileSnapshot WatchFileSnapshot::Capture(const std::filesystem::path& file_path) {
    WatchFileSnapshot out;
    out.path = file_path;

    std::error_code ec;
    const auto wt = std::filesystem::last_write_time(file_path, ec);
    if (!ec) {
        out.write_time_ticks = static_cast<std::uint64_t>(wt.time_since_epoch().count());
    }

    ec.clear();
    out.file_size = std::filesystem::file_size(file_path, ec);
    if (ec) {
        out.file_size = 0;
    }

    return out;
}

bool WatchFileSnapshot::operator==(const WatchFileSnapshot& rhs) const {
    return path == rhs.path
        && write_time_ticks == rhs.write_time_ticks
        && file_size == rhs.file_size;
}
