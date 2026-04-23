#pragma once

#include "watch/watch_file_snapshot.h"

#include <filesystem>
#include <vector>

class WatchFileScanner {
public:
    explicit WatchFileScanner(std::filesystem::path root);

    std::vector<WatchFileSnapshot> Scan() const;

    static bool IsWatchedExtension(const std::filesystem::path& path);

private:
    std::filesystem::path root_;
};
