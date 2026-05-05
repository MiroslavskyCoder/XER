/**
 * This file is part of XER, Device open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#pragma once

#include <functional>
#include <memory>
#include <string>
#include "content/device/vr/vr_device.h"

namespace device::vr {

// Simple 4x4 column-major matrix for pose representation.
struct Pose {
    float matrix[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };
};

// VrSession wraps an OpenXR or OpenVR runtime session.
// Automatically selects OpenXR if available, falls back to OpenVR.
class VrSession {
public:
    static VrSession& Instance();

    // Returns the currently attached VR device, or a default struct with
    // is_present=false if no headset is found.
    VrDevice GetDevice() const;

    // Initialize the session for the selected runtime.
    // Returns true on success.
    bool Initialize();

    // Release resources.
    void Shutdown();

    bool IsInitialized() const { return initialized_; }

    // Get the current head pose (view from HMD).
    Pose GetHeadPose() const;

    // Frame lifecycle helpers.
    bool BeginFrame();
    bool EndFrame();

private:
    VrSession() = default;
    bool initialized_ = false;
    VrRuntime active_runtime_ = VrRuntime::kNone;
    void* runtime_handle_ = nullptr;  // dlopen handle for OpenXR lib
};

}  // namespace device::vr
