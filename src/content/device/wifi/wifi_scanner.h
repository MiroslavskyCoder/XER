/**
 * This file is part of XER, Device open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#pragma once

#include <string>
#include <vector>
#include "content/device/wifi/wifi_network.h"

namespace device::wifi {

// WifiScanner reads available networks from the kernel wireless subsystem.
// On Linux it parses /proc/net/wireless for connected-interface stats and
// triggers a passive scan via the nl80211 / iw interface.
class WifiScanner {
public:
    static WifiScanner& Instance();

    // Returns a list of networks found in the most recent scan.
    // Performs a synchronous scan (may block briefly).
    std::vector<WifiNetwork> Scan();

    // Returns the currently connected network for |interface_name| (e.g. "wlan0"),
    // or an empty WifiNetwork if not connected.
    WifiNetwork GetConnectedNetwork(const std::string& interface_name) const;

    // Returns names of all wireless interfaces available on this machine.
    std::vector<std::string> GetInterfaces() const;

private:
    WifiScanner() = default;
};

}  // namespace device::wifi
