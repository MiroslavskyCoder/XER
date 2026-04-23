#include "watch/watch_change_detector.h"

#include <range/v3/algorithm/find_if.hpp>

WatchChangeDetector::SnapshotMap WatchChangeDetector::BuildMap(
    const std::vector<WatchFileSnapshot>& snapshots) {
    SnapshotMap out;
    out.reserve(snapshots.size());
    for (const auto& snap : snapshots) {
        out.emplace(snap.path.string(), snap);
    }
    return out;
}

bool WatchChangeDetector::HasChanges(const SnapshotMap& previous,
                                     const SnapshotMap& current,
                                     std::string* reason) {
    const auto current_change = ranges::find_if(current, [&](const auto& kv) {
        const auto it = previous.find(kv.first);
        return it == previous.end() || !(kv.second == it->second);
    });
    if (current_change != current.end()) {
        const auto prev_it = previous.find(current_change->first);
        if (reason) {
            *reason = (prev_it == previous.end() ? "new file: " : "modified file: ")
                    + current_change->first;
        }
        return true;
    }

    const auto deleted = ranges::find_if(previous, [&](const auto& kv) {
        return current.find(kv.first) == current.end();
    });
    if (deleted != previous.end()) {
        if (reason) *reason = "deleted file: " + deleted->first;
        return true;
    }

    return false;
}
