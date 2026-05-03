#pragma once
#include "video_configuration.h"
#include "video_stream.h"
#include "frame_cache.h"
#include "rendering_engine.h"
#include "compositing_engine.h"
#include "mix_system.h"
#include "mask_system.h"
#include "fps_controller.h"
#include "time_esp_controller.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace video {

/// Central orchestrator for all video operations.
class VideoManager {
public:
    explicit VideoManager(VideoConfiguration cfg = VideoConfiguration::Default());
    ~VideoManager();

    /// Initialize the renderer (creates the backend from RendererType in config).
    bool InitRenderer(RendererType type = RendererType::Vulkan,
                      const std::string& device_hint = "");
    void Shutdown();

    /// Stream management
    VideoStream* CreateStream(const std::string& id);
    VideoStream* GetStream(const std::string& id);
    void         RemoveStream(const std::string& id);

    /// Process a frame: render + composite + mix.
    bool ProcessFrame(Frame& out, const std::string& stream_id);

    // Accessors
    FrameCache&          GetFrameCache()         { return frame_cache_; }
    RenderingEngine*     GetRenderer()           { return renderer_.get(); }
    CompositingEngine&   GetCompositor()         { return compositor_; }
    MixSystem&           GetMixSystem()          { return mix_system_; }
    MaskSystem&          GetMaskSystem()          { return mask_system_; }
    FpsController&       GetFpsController()       { return fps_controller_; }
    TimeEspController&   GetTimeController()      { return time_controller_; }

private:
    VideoConfiguration  cfg_;
    std::unique_ptr<RenderingEngine> renderer_;
    std::unordered_map<std::string, std::unique_ptr<VideoStream>> streams_;
    FrameCache          frame_cache_;
    CompositingEngine   compositor_;
    MixSystem           mix_system_;
    MaskSystem          mask_system_;
    FpsController       fps_controller_;
    TimeEspController   time_controller_;
};

}  // namespace video