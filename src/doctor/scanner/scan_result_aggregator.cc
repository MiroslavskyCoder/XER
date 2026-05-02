#include "scanner/scan_result_aggregator.h"

namespace EngineDoctor {

void ScanResultAggregator::Add(const ScanResult& result) {
    results_.push_back(result);
}

ScanResultAggregator::Summary ScanResultAggregator::GetSummary() const {
    Summary s;
    s.total = static_cast<int>(results_.size());
    for (const auto& r : results_) {
        switch (r.status) {
            case ScanStatus::SUCCESS: ++s.success;  break;
            case ScanStatus::WARNING: ++s.warnings; break;
            case ScanStatus::ERROR:   ++s.errors;   break;
            case ScanStatus::SKIPPED: ++s.skipped;  break;
            default: break;
        }
    }
    return s;
}

std::vector<ScanResult> ScanResultAggregator::GetAll() const {
    return results_;
}

void ScanResultAggregator::Clear() {
    results_.clear();
}

} // namespace EngineDoctor
