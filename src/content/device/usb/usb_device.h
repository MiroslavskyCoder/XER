/**
 * This file is part of XER, Device open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#pragma once

#include <cstdint>
#include <string>

namespace device::usb {

// USB speed classification.
enum class UsbSpeed {
    kUnknown,
    kLow,       // 1.5 Mb/s
    kFull,      // 12 Mb/s
    kHigh,      // 480 Mb/s
    kSuper,     // 5 Gb/s
    kSuperPlus, // 10 Gb/s
};

// Describes a USB device as exposed by the kernel sysfs or udev.
struct UsbDevice {
    // Path in /sys/bus/usb/devices (e.g. "1-1.2").
    std::string sysfs_path;

    uint16_t vendor_id  = 0;
    uint16_t product_id = 0;
    uint8_t  device_class    = 0;
    uint8_t  device_subclass = 0;
    uint8_t  device_protocol = 0;

    std::string manufacturer;   // May be empty
    std::string product;        // May be empty
    std::string serial_number;  // May be empty
    std::string busnum_devnum;  // "bus:dev" string, e.g. "001:003"

    UsbSpeed speed = UsbSpeed::kUnknown;

    bool IsValid() const { return vendor_id != 0 || product_id != 0; }
};

}  // namespace device::usb
