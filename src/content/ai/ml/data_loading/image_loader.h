#pragma once

#include "data_loader.h"

namespace Engine::MLData::Loading {

class ImageLoader : public DataLoader {
public:
    ImageLoader();
    
    std::shared_ptr<Types::Dataset> LoadDataset(const std::string& path) override;
    void SetImageSize(uint32_t width, uint32_t height);

private:
    uint32_t image_width_;
    uint32_t image_height_;
};

} // namespace Engine::MLData::Loading
