#include "scanner/directory_scanner.h"
#include "scanner/scan_filter.h"

#include <filesystem>
#include <system_error>

namespace EngineDoctor {

void DirectoryScanner::SetFilter(std::shared_ptr<ScanFilter> filter) {
    filter_ = std::move(filter);
}

std::vector<ScanResult> DirectoryScanner::Scan(
        const std::string& root_path,
        const ScanParameters& params) {
    std::vector<ScanResult> results;
    std::error_code ec;

    const std::filesystem::path root(root_path);
    if (!std::filesystem::exists(root, ec)) {
        ScanResult r;
        r.file_path = root_path;
        r.metadata.full_path = root_path;
        r.status = ScanStatus::ERROR;
        r.error_message = "Root path does not exist.";
        r.errors.emplace_back("PATH_NOT_FOUND", r.error_message);
        results.push_back(std::move(r));
        return results;
    }

    std::filesystem::recursive_directory_iterator it(
        root,
        std::filesystem::directory_options::skip_permission_denied,
        ec);

    for (; it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) {
            ec.clear();
            continue;
        }

        const auto& entry = *it;
        const std::string filename = entry.path().filename().string();

        if (!params.include_hidden && !filename.empty() && filename.front() == '.') {
            if (entry.is_directory(ec)) {
                it.disable_recursion_pending();
            }
            continue;
        }

        if (!entry.is_regular_file(ec)) continue;

        const std::string file_path = entry.path().string();
        if (filter_ && !filter_->Matches(file_path)) continue;

        ScanResult r;
        r.file_path = file_path;
        r.metadata.full_path = file_path;
        r.metadata.exists = true;
        r.metadata.is_directory = false;
        r.metadata.extension = entry.path().extension().string();
        r.metadata.size = std::filesystem::file_size(entry.path(), ec);

        if (ec) {
            r.status = ScanStatus::ERROR;
            r.error_message = ec.message();
            r.errors.emplace_back("FILE_SIZE_FAILED", ec.message());
            ec.clear();
        } else {
            r.status = ScanStatus::SUCCESS;
        }

        results.push_back(std::move(r));
    }

    return results;
}

} // namespace EngineDoctor
