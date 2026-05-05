/**
 * This file is part of XER, Device open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "content/device/vr/vr_device.h"
#include <functional>

namespace device::vr {

// Axis value in [-1, 1].
struct AxisState {
    float x = 0.f;
    float y = 0.f;
};

// The state of a single VR controller.
struct ControllerState {
    uint32_t controller_index = 0;

    // Trigger (0=released, 1=fully pressed).
    float trigger        = 0.f;
    float grip           = 0.f;

    // Thumbstick / touchpad.
    AxisState thumbstick;

    // Digital buttons (bitmask).
    uint32_t button_mask = 0;

    bool is_tracked      = false;
    Pose pose;             // Controller pose in world space

    // Convenience: test button by bit index.
    bool IsButtonPressed(uint8_t bit) const {
        return (button_mask & (1u << bit)) != 0;
    }
};

enum class HapticIntensity { kLight, kMedium, kStrong };

// VrInput polls and dispatches VR controller and HMD input.
// Integrates with VrSession; uses OpenXR xrSyncActions when available.
class VrInput {
public:
    static VrInput& Instance();

    // Initialize action sets (requires VrSession to be initialized first).
    bool Initialize();
    void Shutdown();
    bool IsInitialized() const { return initialized_; }

    // Poll the latest controller states.  Must be called once per frame
    // after VrSession::BeginFrame().
    void PollInput();

    // Returns the controller state snapshot for |index| (0=left, 1=right).
    ControllerState GetController(uint32_t index) const;

    // Returns the current HMD pose (same as VrSession::GetHeadPose() but
    // refreshed here so all input is collected in one place).
    Pose GetHeadPose() const;

    // Trigger haptic feedback on controller |index|.
    bool TriggerHaptic(uint32_t index,
                       HapticIntensity intensity = HapticIntensity::kMedium,
                       uint32_t duration_ms = 50);

    // Button / trigger change callbacks.
    using ButtonCallback = std::function<void(uint32_t controller,
                                               uint8_t  button,
                                               bool     pressed)>;
    void SetButtonCallback(ButtonCallback cb) { button_cb_ = std::move(cb); }

private:
    VrInput() = default;
    bool initialized_ = false;
    ControllerState controllers_[2];
    ButtonCallback  button_cb_;
    uint32_t        prev_button_masks_[2] = {0, 0};
};

}  // namespace device::vr
