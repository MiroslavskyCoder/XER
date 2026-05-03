#include "vulkan_renderer.h"
#include <cstring>

namespace video {

VulkanRenderer::~VulkanRenderer() { Shutdown(); }

bool VulkanRenderer::Initialize(const std::string& /*device_hint*/) {
    // TODO: create VkInstance, VkDevice, command pool, etc.
    ready_ = true;
    return true;
}

void VulkanRenderer::Shutdown() { ready_ = false; }

bool VulkanRenderer::Render(const Frame& src, RenderTarget& target) {
    if (!ready_ || !src.IsValid()) return false;
    Frame* dst = target.GetFrame();
    if (!dst || !dst->IsValid()) return false;
    std::memcpy(dst->Data(), src.Data(), src.Width() * src.Height() * 4);
    return true;
}

}  // namespace video