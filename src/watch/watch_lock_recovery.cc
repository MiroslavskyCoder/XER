#include "watch/watch_lock_recovery.h"

#include <system_error>

namespace {

std::filesystem::path LockPathForScript(const std::filesystem::path& script_path) {
    return script_path.parent_path() /
           (script_path.filename().string() + ".flowcache.lock");
}

}  // namespace

bool WatchLockRecovery::CleanupStaleLockForScript(const std::filesystem::path& script_path,
                                                  std::string* error_message) {
    const auto lock_path = LockPathForScript(script_path);

    std::error_code ec;
    if (!std::filesystem::exists(lock_path, ec)) {
        return true;
    }

    ec.clear();
    std::filesystem::remove_all(lock_path, ec);
    if (ec) {
        if (error_message) {
            *error_message = "Failed to remove stale lock: " + lock_path.string() + ", reason: " + ec.message();
        }
        return false;
    }

    return true;
}
