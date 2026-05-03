#include "video_manager.h"
#include <cstring>
#include "cuda_renderer.h"
#include "vulkan_renderer.h"
#include "angle_renderer.h"

namespace video {

VideoManager::VideoManager(VideoConfiguration cfg)
    : cfg_(std::move(cfg)),
      frame_cache_(cfg_.max_cache_frames),
      fps_controller_(cfg_.target_fps) {}

VideoManager::~VideoManager() { Shutdown(); }

bool VideoManager::InitRenderer(RendererType type, const std::string& device_hint) {
    switch (type) {
        case RendererType::CUDA:   renderer_ = std::make_unique<CudaRenderer>();   break;
        case RendererType::Vulkan: renderer_ = std::make_unique<VulkanRenderer>(); break;
        case RendererType::Angle:  renderer_ = std::make_unique<AngleRenderer>();  break;
        default:                   renderer_ = std::make_unique<VulkanRenderer>(); break;
    }
    return renderer_->Initialize(device_hint);
}

void VideoManager::Shutdown() {
    for (auto& [id, s] : streams_) s->Stop();
    streams_.clear();
    if (renderer_) renderer_->Shutdown();
}

VideoStream* VideoManager::CreateStream(const std::string& id) {
    auto [it, ok] = streams_.emplace(id, std::make_unique<VideoStream>(id));
    return it->second.get();
}

VideoStream* VideoManager::GetStream(const std::string& id) {
    auto it = streams_.find(id);
    return (it != streams_.end()) ? it->second.get() : nullptr;
}

void VideoManager::RemoveStream(const std::string& id) {
    streams_.erase(id);
}

bool VideoManager::ProcessFrame(Frame& out, const std::string& stream_id) {
    VideoStream* stream = GetStream(stream_id);
    if (!stream) return false;

    auto frame = stream->NextFrame();
    if (!frame) return false;

    if (renderer_ && renderer_->IsReady()) {
        RenderTarget rt(frame->Width(), frame->Height(), frame->Format());
        if (!renderer_->Render(*frame, rt)) return false;
        if (rt.GetFrame()) {
            out.Allocate(rt.Width(), rt.Height(), rt.Format());
            std::memcpy(out.Data(), rt.GetFrame()->Data(),
                        rt.Width() * rt.Height() * 4);
        }
    } else {
        out.Allocate(frame->Width(), frame->Height(), frame->Format());
        std::memcpy(out.Data(), frame->Data(),
                    frame->Width() * frame->Height() * 4);
    }
    return true;
}

}  // namespace video