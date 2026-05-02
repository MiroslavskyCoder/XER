#pragma once

#include <memory>
#include <string>
#include <vector>
#include "scanner/scan_parameters.h"
#include "scanner/scan_result.h"

namespace EngineDoctor {

class ScanFilter;

class DirectoryScanner {
public:
    std::vector<ScanResult> Scan(const std::string& root_path, const ScanParameters& params);
    void SetFilter(std::shared_ptr<ScanFilter> filter);

private:
    std::shared_ptr<ScanFilter> filter_;
};

} // namespace EngineDoctor
