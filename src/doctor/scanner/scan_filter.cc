#include "scanner/scan_filter.h"

#include <algorithm>
#include <filesystem>
#include <regex>

namespace EngineDoctor {

void ScanFilter::AddIncludePattern(std::string pattern) {
    include_patterns_.push_back(std::move(pattern));
}

void ScanFilter::AddExcludePattern(std::string pattern) {
    exclude_patterns_.push_back(std::move(pattern));
}

void ScanFilter::AddExtension(std::string ext) {
    extensions_.push_back(std::move(ext));
}

bool ScanFilter::Matches(const std::string& path) const {
    // Extension whitelist: if any are registered, path must match one
    if (!extensions_.empty()) {
        const std::string ext = std::filesystem::path(path).extension().string();
        const bool ext_ok = std::any_of(
            extensions_.begin(), extensions_.end(),
            [&ext](const std::string& e) { return e == ext; });
        if (!ext_ok) return false;
    }

    // Exclude patterns: any match means the path is rejected
    for (const auto& pattern : exclude_patterns_) {
        try {
            const std::regex re(pattern, std::regex::ECMAScript | std::regex::icase);
            if (std::regex_search(path, re)) return false;
        } catch (const std::regex_error&) {
            // malformed pattern — skip silently
        }
    }

    // Include patterns: if any are registered, at least one must match
    if (!include_patterns_.empty()) {
        const bool any_match = std::any_of(
            include_patterns_.begin(), include_patterns_.end(),
            [&path](const std::string& pattern) {
                try {
                    const std::regex re(pattern, std::regex::ECMAScript | std::regex::icase);
                    return std::regex_search(path, re);
                } catch (const std::regex_error&) {
                    return false;
                }
            });
        if (!any_match) return false;
    }

    return true;
}

} // namespace EngineDoctor
