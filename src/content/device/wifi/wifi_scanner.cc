/**
 * This file is part of XER, Device open source project.
 * Copyright (C) 2026 Yoshi A.
 */
#include "content/device/wifi/wifi_scanner.h"

#include <cstring>
#include <fstream>
#include <sstream>

#if defined(__linux__)
#include <dirent.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/wireless.h>
#include <unistd.h>
#endif
#include <cmath>

namespace device::wifi {

namespace {

#if defined(__linux__)
// Parse /proc/net/wireless — returns signal for each interface.
// Format (after 2-line header):
//   wlan0: 0000  -50.  -80.   0.       0      0      0      0     0     0
static std::vector<std::pair<std::string, int>> ReadProcNetWireless() {
    std::vector<std::pair<std::string, int>> result;
    std::ifstream f("/proc/net/wireless");
    if (!f.good()) return result;

    std::string line;
    // Skip two header lines.
    std::getline(f, line);
    std::getline(f, line);

    while (std::getline(f, line)) {
        std::istringstream ss(line);
        std::string iface;
        std::string status, link, level;
        ss >> iface >> status >> link >> level;
        if (iface.empty()) continue;
        // Remove trailing colon.
        if (!iface.empty() && iface.back() == ':') iface.pop_back();
        // Remove trailing dot from level.
        if (!level.empty() && level.back() == '.') level.pop_back();
        int sig = 0;
        try { sig = std::stoi(level); } catch (...) {}
        result.push_back({iface, sig});
    }
    return result;
}

// Enumerate wireless interfaces from /sys/class/net/<iface>/wireless.
static std::vector<std::string> ListWirelessInterfaces() {
    std::vector<std::string> result;
    DIR* dir = opendir("/sys/class/net");
    if (!dir) return result;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        const std::string name = entry->d_name;
        if (name.empty() || name[0] == '.') continue;
        // Check for the "wireless" subdirectory.
        const std::string wpath = "/sys/class/net/" + name + "/wireless";
        DIR* wd = opendir(wpath.c_str());
        if (wd) {
            result.push_back(name);
            closedir(wd);
        }
    }
    closedir(dir);
    return result;
}

// Read the current SSID for |iface| using SIOCGIWESSID ioctl.
static std::string ReadCurrentSsid(const std::string& iface) {
    int fd = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (fd < 0) return {};
    struct iwreq req{};
    char essid[IW_ESSID_MAX_SIZE + 1]{};
    strncpy(req.ifr_name, iface.c_str(), IFNAMSIZ - 1);
    req.u.essid.pointer = essid;
    req.u.essid.length  = IW_ESSID_MAX_SIZE;
    req.u.essid.flags   = 0;
    const bool ok = (ioctl(fd, SIOCGIWESSID, &req) == 0);
    close(fd);
    if (!ok) return {};
    essid[IW_ESSID_MAX_SIZE] = '\0';
    return essid;
}

// Read frequency (Hz) for |iface|.
static uint32_t ReadFrequencyMhz(const std::string& iface) {
    int fd = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (fd < 0) return 0;
    struct iwreq req{};
    strncpy(req.ifr_name, iface.c_str(), IFNAMSIZ - 1);
    uint32_t mhz = 0;
    if (ioctl(fd, SIOCGIWFREQ, &req) == 0) {
        double hz = req.u.freq.m * std::pow(10.0, req.u.freq.e);
        mhz = static_cast<uint32_t>(hz / 1'000'000.0);
    }
    close(fd);
    return mhz;
}
#endif  // __linux__

}  // namespace

// ──────────────────────────────────────────────────────────────────────────────

WifiScanner& WifiScanner::Instance() {
    static WifiScanner instance;
    return instance;
}

std::vector<WifiNetwork> WifiScanner::Scan() {
    std::vector<WifiNetwork> result;
#if defined(__linux__)
    const auto ifaces = GetInterfaces();
    for (const auto& iface : ifaces) {
        const std::string ssid = ReadCurrentSsid(iface);
        if (ssid.empty()) continue;
        WifiNetwork net;
        net.interface_name = iface;
        net.ssid = ssid;
        net.frequency_mhz = ReadFrequencyMhz(iface);
        if (net.frequency_mhz > 0 && net.frequency_mhz <= 2484)
            net.channel = static_cast<uint16_t>((net.frequency_mhz - 2412) / 5 + 1);
        else if (net.frequency_mhz >= 5180)
            net.channel = static_cast<uint16_t>((net.frequency_mhz - 5000) / 5);
        net.is_connected = true;
        // Get signal from /proc/net/wireless.
        for (const auto& [name, sig] : ReadProcNetWireless()) {
            if (name == iface) { net.signal_dbm = static_cast<int8_t>(sig); break; }
        }
        result.push_back(std::move(net));
    }
#endif
    return result;
}

WifiNetwork WifiScanner::GetConnectedNetwork(const std::string& interface_name) const {
#if defined(__linux__)
    const std::string ssid = ReadCurrentSsid(interface_name);
    if (ssid.empty()) return {};
    WifiNetwork net;
    net.interface_name = interface_name;
    net.ssid = ssid;
    net.frequency_mhz = ReadFrequencyMhz(interface_name);
    net.is_connected = true;
    for (const auto& [name, sig] : ReadProcNetWireless()) {
        if (name == interface_name) { net.signal_dbm = static_cast<int8_t>(sig); break; }
    }
    return net;
#else
    return {};
#endif
}

std::vector<std::string> WifiScanner::GetInterfaces() const {
#if defined(__linux__)
    return ListWirelessInterfaces();
#else
    return {};
#endif
}

}  // namespace device::wifi
