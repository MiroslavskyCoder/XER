/**
 * This file is part of XER, Device open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#include "content/device/vr/vr_session.h"

#include <cstring>
#include <dlfcn.h>

namespace device::vr {

namespace {

// Minimal OpenXR typedefs needed to detect runtime availability.
using XrResult   = int32_t;
using XrInstance = void*;
struct XrApplicationInfo {
    char applicationName[128];
    uint32_t applicationVersion;
    char engineName[128];
    uint32_t engineVersion;
    uint64_t apiVersion;
};
struct XrInstanceCreateInfo {
    int type; // XR_TYPE_INSTANCE_CREATE_INFO = 1
    const void* next;
    uint64_t createFlags;
    XrApplicationInfo applicationInfo;
    uint32_t enabledApiLayerCount;
    const char* const* enabledApiLayerNames;
    uint32_t enabledExtensionCount;
    const char* const* enabledExtensionNames;
};

using PfnXrCreateInstance =
    XrResult (*)(const XrInstanceCreateInfo*, XrInstance*);
using PfnXrGetInstanceProperties =
    XrResult (*)(XrInstance, void* /*XrInstanceProperties**/);
using PfnXrDestroyInstance = XrResult (*)(XrInstance);

constexpr XrResult kXrSuccess = 0;

static bool ProbeOpenXR(VrDevice& out) {
    void* lib = dlopen("libopenxr_loader.so.1", RTLD_LAZY | RTLD_LOCAL);
    if (!lib) lib = dlopen("libopenxr_loader.so", RTLD_LAZY | RTLD_LOCAL);
    if (!lib) return false;

    auto pfnCreate = reinterpret_cast<PfnXrCreateInstance>(
        dlsym(lib, "xrCreateInstance"));
    if (!pfnCreate) { dlclose(lib); return false; }

    XrInstanceCreateInfo info{};
    info.type = 1 /*XR_TYPE_INSTANCE_CREATE_INFO*/;
    strncpy(info.applicationInfo.applicationName, "XER", 127);
    info.applicationInfo.applicationVersion = 1;
    strncpy(info.applicationInfo.engineName, "XER Engine", 127);
    info.applicationInfo.engineVersion = 1;
    info.applicationInfo.apiVersion = (1ULL << 48); // XR_MAKE_VERSION(1,0,0)

    XrInstance instance = nullptr;
    if (pfnCreate(&info, &instance) != kXrSuccess || !instance) {
        dlclose(lib);
        return false;
    }

    out.runtime      = VrRuntime::kOpenXR;
    out.name         = "OpenXR HMD";
    out.manufacturer = "OpenXR Runtime";
    out.is_present   = true;
    // Default suggested render resolution; a real integration would query
    // xrEnumerateViewConfigurations.
    out.render_width  = 1440;
    out.render_height = 1600;
    out.refresh_rate  = 90.f;

    auto pfnDestroy = reinterpret_cast<PfnXrDestroyInstance>(
        dlsym(lib, "xrDestroyInstance"));
    if (pfnDestroy) pfnDestroy(instance);
    dlclose(lib);
    return true;
}

}  // namespace

// ──────────────────────────────────────────────────────────────────────────────

VrSession& VrSession::Instance() {
    static VrSession inst;
    return inst;
}

VrDevice VrSession::GetDevice() const {
    VrDevice dev;
    ProbeOpenXR(dev);
    return dev;
}

bool VrSession::Initialize() {
    if (initialized_) return true;
    VrDevice dev;
    if (ProbeOpenXR(dev)) {
        active_runtime_ = VrRuntime::kOpenXR;
        initialized_ = true;
        return true;
    }
    return false;
}

void VrSession::Shutdown() {
    initialized_ = false;
    active_runtime_ = VrRuntime::kNone;
    if (runtime_handle_) {
        dlclose(runtime_handle_);
        runtime_handle_ = nullptr;
    }
}

Pose VrSession::GetHeadPose() const {
    // Returns identity pose; a real integration would call
    // xrLocateViews / xrLocateSpace.
    return Pose{};
}

bool VrSession::BeginFrame() {
    return initialized_;
}

bool VrSession::EndFrame() {
    return initialized_;
}

}  // namespace device::vr
