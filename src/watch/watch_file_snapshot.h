#pragma once

#include <cstdint>
#include <filesystem>

struct WatchFileSnapshot {
    std::filesystem::path path;
    std::uint64_t write_time_ticks = 0;
    std::uintmax_t file_size = 0;

    static WatchFileSnapshot Capture(const std::filesystem::path& file_path);

    bool operator==(const WatchFileSnapshot& rhs) const;
};
