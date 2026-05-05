/**
 * This file is part of XER, Device open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#pragma once

#include <cstdint>
#include <string>

namespace device::cast::mira {

// Represents a Miracast / Wi-Fi Direct display sink found during P2P scan.
struct MiraCastDevice {
    std::string friendly_name;
    std::string device_address;   // P2P MAC or IP after DHCP
    std::string wfd_device_info;  // Wi-Fi Display IE subelement string
    uint16_t    rtsp_port = 7236; // Default WFD RTSP control port
    bool        is_available = false;
};

enum class MiraCastState {
    kIdle,
    kDiscovering,
    kConnecting,
    kNegotiating,   // WFD capability exchange via RTSP
    kStreaming,
    kPaused,
    kError,
    kDisconnected,
};

}  // namespace device::cast::mira
