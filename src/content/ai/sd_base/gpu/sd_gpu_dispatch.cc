#include "content/ai/sd_base/gpu/sd_gpu_dispatch.h"

#include "content/ai/models_builder/utility/ai_runtime_features.h"
#include "wrapper/cuda/cuda_engine_bridge.h"
#include "wrapper/ffmpeg/ffmpeg_engine_bridge.h"
#include "wrapper/opencv/opencv_engine_bridge.h"

namespace Engine::AI::SDBase {

SdRuntimeCapabilities SdGpuDispatch::Detect() {
    SdRuntimeCapabilities caps;
    const auto libs = Engine::ModelsBuilder::Utility::DetectExternalLibraries();

    caps.cuda_available = libs.has_cuda && engine::bridge::cuda::IsAvailable();
    caps.opencv_available = libs.has_opencv && engine::bridge::opencv::IsAvailable();
    caps.ffmpeg_available = engine::bridge::ffmpeg::IsAvailable();
    caps.eigen_available = libs.has_eigen;
    caps.xnnpack_available = libs.has_xnnpack;
    return caps;
}

bool SdGpuDispatch::ShouldUseCuda(const SdRuntimeCapabilities& caps, bool prefer_cuda) {
    return prefer_cuda && caps.cuda_available;
}

}  // namespace Engine::AI::SDBase
