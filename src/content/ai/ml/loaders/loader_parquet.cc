#include "loader_parquet.h"

#include <absl/strings/string_view.h>
#include <range/v3/view.hpp>

namespace Engine::ML::Loaders {

Dataset LoaderParquet::Load(const std::string& path) {
    Dataset cached;
    if (TryLoadCachedDataset(Name(), path, &cached)) {
        return cached;
    }

    // Stub: requires Apache Arrow/Parquet at link time
    Dataset ds{};
    StoreCachedDataset(Name(), path, ds);
    return ds;
}

}  // namespace Engine::ML::Loaders
