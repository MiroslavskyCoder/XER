/**
 * This file is part of XER, Device open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#pragma once

#include <functional>
#include <string>
#include <vector>
#include "content/device/usb/usb_device.h"

namespace device::usb {

using HotplugCallback = std::function<void(const UsbDevice& dev, bool attached)>;

// Enumerates USB devices present in the system and monitors hotplug events.
// Uses the Linux sysfs tree (/sys/bus/usb/devices) for enumeration.
class UsbEnumerator {
public:
    static UsbEnumerator& Instance();

    // Returns all currently connected USB devices.
    std::vector<UsbDevice> EnumerateDevices() const;

    // Register a callback to be invoked when a device is attached or removed.
    // Monitoring is done via inotify on /sys/bus/usb/devices (best-effort).
    void SetHotplugCallback(HotplugCallback callback);

    // Start background hotplug monitoring (non-blocking).
    bool StartMonitoring();
    void StopMonitoring();

private:
    UsbEnumerator() = default;
    HotplugCallback hotplug_cb_;
    bool monitoring_ = false;
};

}  // namespace device::usb
