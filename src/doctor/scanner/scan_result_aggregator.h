#pragma once

#include <vector>
#include "scanner/scan_result.h"

namespace EngineDoctor {

class ScanResultAggregator {
public:
    struct Summary {
        int total   = 0;
        int success = 0;
        int warnings = 0;
        int errors  = 0;
        int skipped = 0;
    };

    void Add(const ScanResult& result);
    Summary GetSummary() const;
    std::vector<ScanResult> GetAll() const;
    void Clear();

private:
    std::vector<ScanResult> results_;
};

} // namespace EngineDoctor
