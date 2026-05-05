/**
 * This file is part of XER, Device open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#pragma once

#include <cstdint>
#include <string>

namespace device::bluetooth {

enum class BluetoothDeviceType {
    kUnknown,
    kComputer,
    kPhone,
    kHeadset,
    kHeadphones,
    kKeyboard,
    kMouse,
    kGamepad,
    kPeripheral,
};

// Represents a remote Bluetooth device discovered during a scan or from the
// paired-devices list.
struct BluetoothDevice {
    std::string address;       // "AA:BB:CC:DD:EE:FF"
    std::string name;          // Human-readable name (may be empty)
    BluetoothDeviceType type = BluetoothDeviceType::kUnknown;
    int8_t rssi = 0;           // Signal strength in dBm (0 = unknown)
    bool is_paired = false;
    bool is_connected = false;

    // Convenience
    bool IsValid() const { return !address.empty(); }
};

}  // namespace device::bluetooth
