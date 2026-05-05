/**
 * This file is part of XER, Device open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#pragma once

#include <cstdint>
#include <string>

namespace device::vr {

enum class VrRuntime {
    kNone,
    kOpenXR,    // Khronos OpenXR (preferred)
    kOpenVR,    // Valve SteamVR
};

enum class VrTrackingState {
    kNotTracked,
    kLimited,
    kTracking,
};

// Represents a connected VR headset / display.
struct VrDevice {
    std::string name;
    std::string manufacturer;
    VrRuntime   runtime  = VrRuntime::kNone;
    uint32_t    render_width  = 0;  // Per-eye width in pixels
    uint32_t    render_height = 0;  // Per-eye height in pixels
    float       refresh_rate  = 0.f; // Hz
    bool        is_present    = false;
    VrTrackingState tracking_state = VrTrackingState::kNotTracked;
};

}  // namespace device::vr
