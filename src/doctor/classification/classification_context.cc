#include "classification_context.h"

namespace EngineDoctor {

void ClassificationContext::Set(const std::string& key, const std::string& val) {
    metadata[key] = val;
}

std::string ClassificationContext::Get(const std::string& key) const {
    const auto it = metadata.find(key);
    if (it != metadata.end()) {
        return it->second;
    }
    return {};
}

bool ClassificationContext::HasKey(const std::string& key) const {
    return metadata.count(key) > 0;
}

} // namespace EngineDoctor
