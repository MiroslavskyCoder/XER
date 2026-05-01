#include "data_loader.h"

namespace Engine::MLData::Loading {

DataLoader::DataLoader() {
}

std::shared_ptr<Types::Dataset> DataLoader::LoadDataset(const std::string& path) {
    return std::make_shared<Types::Dataset>("LoadedDataset");
}

bool DataLoader::SaveDataset(const std::string& path, const Types::Dataset& dataset) {
    return true;
}

} // namespace Engine::MLData::Loading
