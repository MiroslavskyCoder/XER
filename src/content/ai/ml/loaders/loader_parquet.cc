#include "loader_parquet.h"

namespace Engine::ML::Loaders {

Dataset LoaderParquet::Load(const std::string& /*path*/) {
    // Stub: requires Apache Arrow/Parquet at link time
    return Dataset{};
}

}  // namespace Engine::ML::Loaders
