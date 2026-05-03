#include "loader_csv.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace Engine::ML::Loaders {

Dataset LoaderCsv::Load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("CsvLoader: cannot open " + path);

    Dataset ds;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        std::string cell;
        std::vector<float> row;
        while (std::getline(ss, cell, delimiter_)) {
            try { row.push_back(std::stof(cell)); }
            catch (...) { row.push_back(0.0f); }
        }
        if (row.empty()) continue;
        if (has_labels_) {
            ds.y.push_back(static_cast<int>(row.back()));
            row.pop_back();
        }
        ds.X.push_back(std::move(row));
    }
    return ds;
}

}  // namespace Engine::ML::Loaders
