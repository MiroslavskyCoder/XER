#include "image_loader.h"

namespace Engine::MLData::Loading {

ImageLoader::ImageLoader() : image_width_(224), image_height_(224) {
}

std::shared_ptr<Types::Dataset> ImageLoader::LoadDataset(const std::string& path) {
    return std::make_shared<Types::Dataset>("ImageDataset");
}

void ImageLoader::SetImageSize(uint32_t width, uint32_t height) {
    image_width_ = width;
    image_height_ = height;
}

} // namespace Engine::MLData::Loading
