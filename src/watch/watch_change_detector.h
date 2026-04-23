#pragma once

#include "watch/watch_file_snapshot.h"

#include <string>
#include <unordered_map>
#include <vector>

class WatchChangeDetector {
public:
    using SnapshotMap = std::unordered_map<std::string, WatchFileSnapshot>;

    static SnapshotMap BuildMap(const std::vector<WatchFileSnapshot>& snapshots);

    static bool HasChanges(const SnapshotMap& previous,
                           const SnapshotMap& current,
                           std::string* reason);
};
