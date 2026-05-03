#pragma once
#include "video_constants.h"
#include <string>

namespace video {

struct VideoConfiguration {
    RendererType renderer{RendererType::Vulkan};
    int  target_fps{kDefaultFps};
    int  max_cache_frames{kMaxFrameCacheSize};
    bool hw_decode{true};
    bool hw_encode{true};
    std::string cache_dir{"/tmp/xer_video_cache"};

    static VideoConfiguration& Default() {
        static VideoConfiguration cfg;
        return cfg;
    }
};

}  // namespace video