#include "loader_protobuf.h"

namespace Engine::ML::Loaders {

Dataset LoaderProtobuf::Load(const std::string& /*path*/) {
    // Stub: requires libprotobuf at link time
    return Dataset{};
}

}  // namespace Engine::ML::Loaders
