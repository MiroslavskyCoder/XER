#include "csv_loader.h"

namespace Engine::MLData::Loading {

CSVLoader::CSVLoader() : delimiter_(',') {
}

std::shared_ptr<Types::Dataset> CSVLoader::LoadDataset(const std::string& path) {
    return std::make_shared<Types::Dataset>("CSV_Dataset");
}

bool CSVLoader::SaveDataset(const std::string& path, const Types::Dataset& dataset) {
    return true;
}

} // namespace Engine::MLData::Loading
