#pragma once

#include <filesystem>
#include <string>

class WatchLockRecovery {
public:
    // Removes stale .flowcache.lock directory/file for the given script via the shared cache lock API.
    // Returns true when lock is absent or was removed successfully.
    static bool CleanupStaleLockForScript(const std::filesystem::path& script_path,
                                          std::string* error_message);
};
