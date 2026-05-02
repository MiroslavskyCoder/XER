#include "scanner/scan_context.h"

namespace EngineDoctor {

void ScanContext::MarkVisited(const std::string& path) {
    visited_.insert(path);
}

bool ScanContext::IsVisited(const std::string& path) const {
    return visited_.count(path) > 0;
}

void ScanContext::Reset() {
    root_path.clear();
    current_depth = 0;
    visited_.clear();
}

} // namespace EngineDoctor
