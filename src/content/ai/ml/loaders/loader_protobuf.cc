#include "loader_protobuf.h"

#include <absl/strings/string_view.h>
#include <range/v3/view.hpp>

namespace Engine::ML::Loaders {

Dataset LoaderProtobuf::Load(const std::string& path) {
    Dataset cached;
    if (TryLoadCachedDataset(Name(), path, &cached)) {
        return cached;
    }

    // Stub: requires libprotobuf at link time
    Dataset ds{};
    StoreCachedDataset(Name(), path, ds);
    return ds;
}

}  // namespace Engine::ML::Loaders
