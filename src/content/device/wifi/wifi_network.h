/**
 * This file is part of XER, Device open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#pragma once

#include <cstdint>
#include <string>

namespace device::wifi {

enum class WifiSecurityType {
    kOpen,
    kWEP,
    kWPA,
    kWPA2,
    kWPA3,
    kUnknown,
};

// Represents a Wi-Fi access point discovered during a scan.
struct WifiNetwork {
    std::string ssid;
    std::string bssid;          // "AA:BB:CC:DD:EE:FF"
    std::string interface_name; // e.g. "wlan0"
    int8_t      signal_dbm  = 0;
    uint32_t    frequency_mhz = 0;
    uint16_t    channel = 0;
    WifiSecurityType security = WifiSecurityType::kUnknown;
    bool is_connected = false;
};

}  // namespace device::wifi
